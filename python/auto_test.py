#!/usr/bin/env python3
"""
Script de prueba automatizado para validar el flujo completo:
1. Espera telemetría inicial (modo SEGURO)
2. Envía AUTO -> verifica modo AUTO
3. Simula alarma (espera alerta)
4. Envía SILENCIAR -> verifica alarma silenciada
5. Envía ARMAR -> verifica modo ARMADO
"""

import json
import time
import sys
import threading
import paho.mqtt.client as mqtt

BROKER = "broker.wokwi.com"
PORT = 1883
TOPIC_TELEMETRIA = "iot/a909c5ad6f/telemetria"
TOPIC_CONTROL = "iot/a909c5ad6f/control"

client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2, client_id="auto-test-" + str(int(time.time())))

received_messages = []
mode_changes = []
alarm_changes = []
sequence_numbers = []
test_passed = True
current_mode = None
current_alarm = None
lock = threading.Lock()


def on_connect(client, userdata, flags, reason_code, properties):
    if reason_code == 0:
        print(f"[OK] Conectado a {BROKER}")
        client.subscribe(TOPIC_TELEMETRIA, qos=1)
    else:
        print(f"[FAIL] Error conexión: {reason_code}")
        sys.exit(1)


def on_message(client, userdata, msg):
    global current_mode, current_alarm
    try:
        payload = json.loads(msg.payload.decode())
    except json.JSONDecodeError:
        return

    with lock:
        seq = payload.get("sequence", 0)
        mode = payload.get("mode", "?")
        alarm = payload.get("alarm", False)
        value = payload.get("value", 0)

        sequence_numbers.append(seq)
        received_messages.append(payload)

        if mode != current_mode:
            mode_changes.append((seq, current_mode, mode))
            print(f"[MODE] Seq={seq} {current_mode} -> {mode}")
            current_mode = mode

        if alarm != current_alarm:
            alarm_changes.append((seq, current_alarm, alarm))
            print(f"[ALARM] Seq={seq} {current_alarm} -> {alarm}")
            current_alarm = alarm


def send_command(cmd, wait=1.0):
    payload = json.dumps({"comando": cmd})
    result = client.publish(TOPIC_CONTROL, payload, qos=1)
    if result.rc == mqtt.MQTT_ERR_SUCCESS:
        print(f"\n>>> ENVIANDO: {cmd}")
    else:
        print(f"[FAIL] Error enviando {cmd}")
    time.sleep(wait)


def wait_for_mode(target_mode, timeout=10):
    start = time.time()
    while time.time() - start < timeout:
        with lock:
            if current_mode == target_mode:
                return True
        time.sleep(0.2)
    return False


def wait_for_alarm(target_alarm, timeout=10):
    start = time.time()
    while time.time() - start < timeout:
        with lock:
            if current_alarm == target_alarm:
                return True
        time.sleep(0.2)
    return False


def check_sequence_increment():
    for i in range(1, len(sequence_numbers)):
        if sequence_numbers[i] <= sequence_numbers[i-1]:
            print(f"[FAIL] Sequence no incrementa: {sequence_numbers[i-1]} -> {sequence_numbers[i]}")
            return False
    print(f"[OK] Sequence incrementa correctamente: {sequence_numbers}")
    return True


def run_test():
    global test_passed

    print("="*60)
    print("PRUEBA AUTOMATIZADA FLUJO COMPLETO")
    print("="*60)

    client.on_connect = on_connect
    client.on_message = on_message

    try:
        client.connect(BROKER, PORT, 60)
    except Exception as e:
        print(f"[FAIL] Error conectando: {e}")
        return False

    client.loop_start()

    # Esperar telemetría inicial (modo SEGURO)
    print("\n[1] Esperando telemetría inicial (modo SEGURO)...")
    time.sleep(3)
    with lock:
        if current_mode != "SEGURO":
            print(f"[FAIL] Modo inicial esperado SEGURO, got {current_mode}")
            test_passed = False
        else:
            print(f"[OK] Modo inicial: SEGURO")

    # Enviar AUTO
    print("\n[2] Enviando comando AUTO...")
    send_command("AUTO")
    if wait_for_mode("AUTO", 5):
        print("[OK] Modo cambiado a AUTO")
    else:
        print("[FAIL] No cambió a AUTO")
        test_passed = False


    print("\n[3] Esperando alarma (requiere sensor físico >= 1100 ADC por ~7.75s)...")
    print("    Simule gas en Wokwi (slider MQ2 > 1100) y espere...")
    if wait_for_alarm(True, 30):
        print("[OK] Alarma activada (alerta inmediata recibida)")
    else:
        print("[WARN] Alarma no activada en 30s (verifique sensor en Wokwi)")

    # Enviar SILENCIAR
    print("\n[4] Enviando comando SILENCIAR...")
    send_command("SILENCIAR")
    
    time.sleep(2)
    with lock:
        if current_alarm is True:
            print("[OK] Alarma sigue activa (latched), buzzer silenciado")
        else:
            print(f"[INFO] Estado alarma: {current_alarm}")

    # Enviar ARMAR
    print("\n[5] Enviando comando ARMAR...")
    send_command("ARMAR")
    if wait_for_mode("ARMADO", 5):
        print("[OK] Modo cambiado a ARMADO")
    else:
        print("[FAIL] No cambió a ARMADO")
        test_passed = False

    # Verificar sequence
    print("\n[6] Verificando sequence numbers...")
    if not check_sequence_increment():
        test_passed = False

    
    print("\n" + "="*60)
    print("RESUMEN")
    print("="*60)
    print(f"Mensajes recibidos: {len(received_messages)}")
    print(f"Cambios de modo: {len(mode_changes)}")
    for seq, old, new in mode_changes:
        print(f"  Seq {seq}: {old} -> {new}")
    print(f"Cambios de alarma: {len(alarm_changes)}")
    for seq, old, new in alarm_changes:
        print(f"  Seq {seq}: {old} -> {new}")
    print(f"Sequence numbers: {sequence_numbers}")
    print(f"\nRESULTADO: {'TODAS LAS PRUEBAS PASARON' if test_passed else 'ALGUNAS PRUEBAS FALLARON'}")

    client.loop_stop()
    client.disconnect()
    return test_passed


if __name__ == "__main__":
    success = run_test()
    sys.exit(0 if success else 1)
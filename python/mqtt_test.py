#!/usr/bin/env python3
"""
Demuestra: telemetría, alertas, comandos y confirmación de estado
"""

import json
import time
import sys
import paho.mqtt.client as mqtt

BROKER = "broker.wokwi.com"
PORT = 1883
TOPIC_TELEMETRIA = "iot/a909c5ad6f/telemetria"
TOPIC_CONTROL = "iot/a909c5ad6f/control"

client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2, client_id="python-test-" + str(int(time.time())))
telemetry_count = 0
alert_count = 0
last_mode = None
last_alarm = None
last_sequence = 0


def on_connect(client, userdata, flags, reason_code, properties):
    if reason_code == 0:
        print(f"[MQTT] Conectado a {BROKER}:{PORT}")
        client.subscribe(TOPIC_TELEMETRIA, qos=1)
        print(f"[MQTT] Suscrito a {TOPIC_TELEMETRIA}")
    else:
        print(f"[MQTT] Error conexión: {reason_code}")
        sys.exit(1)


def on_message(client, userdata, msg):
    global telemetry_count, alert_count, last_mode, last_alarm, last_sequence
    try:
        payload = json.loads(msg.payload.decode())
    except json.JSONDecodeError:
        print(f"[RX] Payload no JSON: {msg.payload}")
        return

    seq = payload.get("sequence", 0)
    mode = payload.get("mode", "?")
    alarm = payload.get("alarm", False)
    value = payload.get("value", 0)
    device = payload.get("device_id", "?")
    variable = payload.get("variable", "?")

    if alarm:
        alert_count += 1
        print(f"\n[ALERTA #{alert_count}] Seq={seq} | {device} | {variable}={value} | mode={mode} | ALARM=True")
    else:
        telemetry_count += 1
        if telemetry_count <= 3 or telemetry_count % 10 == 0:
            print(f"[TELEMETRÍA #{telemetry_count}] Seq={seq} | {device} | {variable}={value} | mode={mode} | alarm={alarm}")

    if mode != last_mode:
        print(f"  >> Cambio de modo: {last_mode} -> {mode}")
        last_mode = mode
    if alarm != last_alarm:
        print(f"  >> Cambio alarma: {last_alarm} -> {alarm}")
        last_alarm = alarm
    last_sequence = seq


def send_command(cmd):
    payload = json.dumps({"comando": cmd})
    result = client.publish(TOPIC_CONTROL, payload, qos=1)
    if result.rc == mqtt.MQTT_ERR_SUCCESS:
        print(f"\n[TX] Comando enviado: {cmd}")
    else:
        print(f"[TX] Error enviando {cmd}: {result.rc}")


def print_menu():
    print("\n" + "="*60)
    print("COMANDOS DISPONIBLES:")
    print("  1 - AUTO      (activa monitoreo, resetea alarma)")
    print("  2 - ARMAR     (arma el sistema, habilita alarma)")
    print("  3 - SILENCIAR (silencia buzzer, alarma latched)")
    print("  4 - Enviar JSON inválido (prueba rechazo)")
    print("  5 - Enviar comando desconocido (prueba rechazo)")
    print("  q - Salir")
    print("="*60)


def main():
    print("=== Test MQTT IOT-A909C5AD6F ===")
    print(f"Broker: {BROKER}:{PORT}")
    print(f"Telemetría: {TOPIC_TELEMETRIA}")
    print(f"Control: {TOPIC_CONTROL}")

    client.on_connect = on_connect
    client.on_message = on_message

    try:
        client.connect(BROKER, PORT, 60)
    except Exception as e:
        print(f"Error conectando: {e}")
        sys.exit(1)

    client.loop_start()

    try:
        while True:
            print_menu()
            choice = input("Opción: ").strip().lower()

            if choice == "1":
                send_command("AUTO")
            elif choice == "2":
                send_command("ARMAR")
            elif choice == "3":
                send_command("SILENCIAR")
            elif choice == "4":
                print("\n[TX] Enviando JSON inválido...")
                client.publish(TOPIC_CONTROL, "{invalido", qos=1)
            elif choice == "5":
                print("\n[TX] Enviando comando desconocido...")
                client.publish(TOPIC_CONTROL, '{"comando": "INEXISTENTE"}', qos=1)
            elif choice == "q":
                break
            else:
                print("Opción inválida")

            time.sleep(0.5)

    except KeyboardInterrupt:
        pass
    finally:
        print("\n[STATS] Telemetría:", telemetry_count, "| Alertas:", alert_count)
        client.loop_stop()
        client.disconnect()
        print("Desconectado.")


if __name__ == "__main__":
    main()
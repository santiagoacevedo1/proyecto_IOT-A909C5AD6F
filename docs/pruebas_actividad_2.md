# Pruebas Actividad 2 - Proyecto IOT-A909C5AD6F

## 1. Funcionalidad Local (Heredados Actividad 1)

| ID | Escenario | Entrada | Resultado Esperado | Estado |
|----|-----------|---------|-------------------|--------|
| LP-01 | Inicialización | Power-on | OLED: "PROYECTO / IOT-A909C5AD6F / INICIANDO", Buzzer beep 200ms, Serial: banner | ✓ |
| LP-02 | Lectura válida normal | MQ2 < 1100 ADC | OLED: "SEGURO", Buzzer OFF, Serial: estado=SEGURO | ✓ |
| LP-03 | Lectura en umbral exacto | MQ2 = 1100 ADC | Contador++ (1/5), Sin alarma aún | ✓ |
| LP-04 | Alarma confirmada | 5 lecturas consecutivas ≥ 1100 | OLED: "ALARMA!", Buzzer ON (2000Hz), Serial: estado=ALARMA | ✓ |
| LP-05 | Histéresis - retorno | Lectura ≤ 1094 tras alarma | Contador=0, Alarma OFF, Buzzer OFF, OLED: "SEGURO" | ✓ |
| LP-06 | Lectura inválida | MQ2 > 4095 o < 0 (simulado) | Serial: valida=no, no afecta contador alarma | ✓ |
| LP-07 | Intervalo muestreo | Tiempo entre lecturas | ~1550 ms ± 50 ms (no bloqueante) | ✓ |

## 2. Casos de Prueba - Conectividad WiFi/MQTT (Nuevos Actividad 2)

| ID | Escenario | Condición | Resultado Esperado | Estado |
|----|-----------|-----------|-------------------|--------|
| CW-01 | Conexión WiFi exitosa | Wokwi simulación | LED: parpadeo rápido → fijo, Serial: "WiFi conectado" | ⬜ |
| CW-02 | Reconexión WiFi | Desconexión simulada | LED: fijo → parpadeo lento → fijo, Serial: "Reconectando..." | ⬜ |
| CW-03 | Conexión MQTT | WiFi OK | LED: parpadeo lento → fijo, Serial: "MQTT conectado" | ⬜ |
| CW-04 | Reconexión MQTT | Broker caído temporal | Reintento cada 5s, LED parpadeo lento durante reconexión | ⬜ |
| CW-05 | Client ID único | Múltiples instancias | `esp32-gas-<MAC>` diferente por dispositivo | ⬜ |

## 3. Casos de Prueba - Telemetría MQTT

| ID | Escenario | Verificación | Estado |
|----|-----------|--------------|--------|
| TM-01 | Publicación periódica | Mensaje cada 1550 ms en `iot/a909c5ad6f/telemetria` | ⬜ |
| TM-02 | Formato JSON válido | Parseable con `jq` o MQTT Explorer | ⬜ |
| TM-03 | Campos obligatorios | proyecto, timestamp_ms, sensor, estado, modo, alarma | ⬜ |
| TM-04 | Valores coherentes | sensor.nivel_gas = lectura ADC actual | ⬜ |
| TM-05 | Estado ALARMA en JSON | `estado: "ALARMA"`, `alarma.confirmada: true` tras 5 lecturas | ⬜ |
| TM-06 | Modo MANUAL en JSON | `modo: "MANUAL"` tras comando | ⬜ |

## 4. Casos de Prueba - Control Remoto (MQTT + Serial)

| ID | Comando | Vía | Resultado Esperado | Estado |
|----|---------|-----|-------------------|--------|
| CR-01 | `{"comando": "AUTO"}` | MQTT | Modo → AUTO, lógica alarma activada | ⬜ |
| CR-02 | `{"comando": "MANUAL"}` | MQTT | Modo → MANUAL, lógica alarma desactivada, buzzer OFF | ⬜ |
| CR-03 | `{"comando": "RESET"}` | MQTT | Estado → SEGURO, contador=0, buzzer OFF | ⬜ |
| CR-04 | Comando `AUTO` | Serial (escribir "AUTO\n") | Igual que CR-01 | ⬜ |
| CR-05 | Comando `MANUAL` | Serial | Igual que CR-02 | ⬜ |
| CR-06 | Comando `RESET` | Serial | Igual que CR-03 | ⬜ |
| CR-07 | Comando inválido | MQTT/Serial | Ignorado, log warning en Serial | ⬜ |

## 5. Casos de Prueba - LED WiFi (GPIO 27)

| ID | Estado Sistema | Patrón LED Esperado | Estado |
|----|----------------|---------------------|--------|
| LW-01 | Inicio, sin WiFi | Apagado | ⬜ |
| LW-02 | Conectando WiFi | Parpadeo rápido (100ms) | ⬜ |
| LW-03 | WiFi OK, MQTT desconectado | Parpadeo lento (1s) | ⬜ |
| LW-04 | WiFi + MQTT OK | Encendido fijo | ⬜ |
| LW-05 | WiFi perdido | Apagado → parpadeo rápido | ⬜ |

## 6. Casos de Prueba - No Bloqueo (Temporización)

| ID | Prueba | Métrica | Criterio Aceptación | Estado |
|----|--------|---------|---------------------|--------|
| NB-01 | Loop duration | `micros()` inicio/fin loop | < 5 ms por iteración (sin muestreo) | ⬜ |
| NB-02 | Muestreo exacto | Diff entre lecturas consecutivas | 1550 ms ± 2% | ⬜ |
| NB-03 | OLED update | Tiempo `oledMostrar()` | < 10 ms | ⬜ |
| NB-04 | MQTT loop | Tiempo `client.loop()` | < 2 ms | ⬜ |
| NB-05 | Serial print | Tiempo `publicarSerial()` | < 5 ms | ⬜ |




## 11. Reporte de Pruebas (Plantilla)

| Fecha | Versión | Probador | Casos Pasados | Casos Fallidos | Observaciones |
|-------|---------|----------|---------------|----------------|---------------|
|       |         |          |               |                |               |
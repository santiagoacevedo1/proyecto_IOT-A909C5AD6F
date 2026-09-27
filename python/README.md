# Scripts de Prueba Python - IOT-A909C5AD6F

## Instalación

```bash
cd python
pip install -r requirements.txt
```

## Scripts

### 1. `mqtt_test.py` - Prueba Interactiva
Menú interactivo para enviar comandos y ver telemetría/alertas en tiempo real.

```bash
python mqtt_test.py
```

**Funciones:**
- Ver telemetría periódica (cada 12s) y alertas inmediatas
- Enviar comandos: `AUTO`, `ARMAR`, `SILENCIAR`
- Probar rechazo de JSON inválido y `COMANDO_DESCONOCIDO`
- Ver cambios de modo y estado de alarma en vivo

### 2. `auto_test.py` - Prueba Automatizada
Valida el flujo completo automáticamente.

```bash
python auto_test.py
```

**Secuencia de prueba:**
1. Verifica modo inicial `SEGURO`
2. Envía `AUTO` → confirma modo `AUTO`
3. Espera alarma (requiere sensor MQ2 ≥ 1100 ADC en Wokwi por ~7.75s)
4. Envía `SILENCIAR` → confirma alarma latched (buzzer off, alarm=true)
5. Envía `ARMAR` → confirma modo `ARMADO`
6. Verifica que `sequence` incrementa correctamente

## Tópicos MQTT

| Tópico | Dirección | Uso |
|--------|-----------|-----|
| `iot/a909c5ad6f/telemetria` | ESP32 → Python | Telemetría periódica + alertas |
| `iot/a909c5ad6f/control` | Python → ESP32 | Comandos: AUTO, ARMAR, SILENCIAR |

## Formato JSON Telemetría/Alerta

```json
{
  "device_id": "IOT-A909C5AD6F",
  "variable": "nivel_gas_MQ2",
  "value": 1150,
  "unit": "ADC",
  "mode": "AUTO",
  "alarm": true,
  "sequence": 42
}
```

## Formato JSON Comando

```json
{"comando": "AUTO"}
{"comando": "ARMAR"}
{"comando": "SILENCIAR"}
```

## Uso en Wokwi

1. Abra el proyecto en Wokwi
2. Inicie la simulación
3. Ejecute los scripts Python
4. En `auto_test.py`: mueva el slider del MQ2 a >1100 cuando se solicite

## Solución de Problemas

- **No conecta**: Verifique internet, broker.wokwi.com:1883 accesible
- **No recibe telemetría**: ESP32 debe estar conectado a WiFi y MQTT (LED azul fijo)
- **Alarma no dispara**: Sensor MQ2 debe leer ≥1100 ADC por 5 lecturas consecutivas (1550ms cada una = ~7.75s)
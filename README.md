# Proyecto IoT - IOT-A909C5AD6F

**Detección de Gas en Cuarto Técnico con Conectividad WiFi/MQTT**
*Internet de las Cosas - Actividades 1 y 2 - IU Digital de Antioquia*

-

## 🏗️ Estructura del Proyecto

```
proyecto_IOT-A909C5AD6F/
├── include/
│   └── secrets.example.h      
├── src/
│   └── main.cpp               
├── docs/
│   ├── interpretacion_asignacion.md    
│   ├── interpretacion_actividad_2.md   
│   ├── arquitectura/
│   │   ├── diagrama_conexiones.drawio
│   │   └── diagrama_bloques.drawio
│   └── pruebas_actividad_2.md         
├── insumos_generador/
│   └── ficha_insumos_individuales.md   
├── platformio.ini            
├── diagram.json              
├── wokwi.toml                 
├── .gitignore                 
└── README.md                  
```

---

## 🔧 Hardware (Wokwi / Real)

| Componente | Pin ESP32 | Tipo Wokwi |
|------------|-----------|------------|
| ESP32 DevKit v1 | - | `wokwi-esp32-devkit-v1` |
| Sensor MQ2 (AO) | GPIO 34 | `wokwi-gas-sensor` |
| OLED SSD1306 (I2C) | GPIO 21 (SDA), 22 (SCL) | `board-ssd1306` |
| Buzzer piezoeléctrico | GPIO 25 | `wokwi-buzzer` |
| LED WiFi (indicador) | GPIO 27 | `wokwi-led`|

**Conexiones I2C OLED**: 3.3V, GND, SDA=21, SCL=22
**MQ2**: 5V, GND, AOUT=34 (ADC1_CH6)
**Buzzer**: GPIO 25 → GND
**LED WiFi**: GPIO 27  → GND

---

## ⚙️ Parámetros Configurables (Actividad 1)

| Parámetro | Valor | Descripción |
|-----------|-------|-------------|
| Umbral principal | 1100 ADC | Activación alarma |
| Intervalo muestreo | 1550 ms | ~2.3 segundos |
| Confirmaciones consecutivas | 5 lecturas | Antes de disparar alarma |
| Histéresis (margen retorno) | 6 ADC | Umbral retorno = 1094 ADC |
| Estado seguro | Buzzer OFF | Excepto alarma confirmada |

---



### 3. Configuración de credenciales
```bash
cp include/secrets.example.h include/secrets.h
** esto es solamente para en caso de usar un Hardware real
```

---

## MQTT 

### Broker por defecto (Wokwi)
```
Broker: broker.wokwi.com
Puerto: 1883
Usuario: (vacío)
Password: (vacío) porque es para Hardware real
en mosquitto no lo realicé, ya que es para Hardware real
```

### Tópicos
| Tópico | Dirección | Formato | Descripción |
|--------|-----------|---------|-------------|
| `iot/a909c5ad6f/telemetria` | ESP32 → Broker | JSON | Datos sensor + estado cada 1.55s |
| `iot/a909c5ad6f/control` | Broker → ESP32 | JSON | Comandos remotos |



---

## PANTALLA OLED

La pantalla muestra 8 líneas :

```
Línea 0: IOT-A909C5AD6F
Línea 1: GAS: 1050
Línea 2: VALIDA: SI
Línea 3: MODO: AUTO
Línea 4: SEGURO          (o ">>> ALARMA! <<<")
Línea 5: CONSEC: 0/5
Línea 6: W:OK M:OK       (WiFi / MQTT)
Línea 7: UP: 123s        (Uptime)
```

---

##  Monitor Serial (115200 baud)

### Salida Periódica (cada muestreo)
```
var=nivel_gas_MQ2 | val=1050 | unit=ADC | valid=si | riesgo=0 | estado=SEGURO | modo=AUTO | wifi=OK | mqtt=OK
```

### Comandos Locales (escribir en Monitor Serial + Enter)
| Comando | Acción |
|---------|--------|
| `AUTO` | Activar modo automático |
| `MANUAL` | Activar modo manual |
| `RESET` | Forzar estado seguro |
| `STATUS` | Mostrar estado completo |
| `HELP` / `?` | Ayuda |

---

## 🔄 Máquina de Estados

Diagrama para la máquina de estados

```
┌─────────────┐     Inicio / RESET      ┌─────────────┐
│  MODO       │ ─────────────────────►  │  MODO       │
│  SEGURO     │                         │  AUTO       │
└─────────────┘                         └──────┬──────┘
       ▲                                       │
       │           Comando MANUAL              │ 5 lecturas ≥ 1100
       │◄──────────────────────────────────────┘
       │                                       ▼
       │                              ┌─────────────────┐
       │                              │ ALARMA_ACTIVA   │
       │                              │ (subestado)     │
       │                              └────────┬────────┘
       │                                       │
       │        Lectura ≤ 1094 (histéresis)    │ Comando RESET
       └───────────────────────────────────────┘
```


---

## 💡 LED WiFi (Conectado a GPIO 27) -

| Patrón | Significado |
|--------|-------------|
| ⬤ Apagado | Sin WiFi |
| ⚡ Parpadeo rápido (100ms) | Conectando WiFi... |
| 🔄 Parpadeo lento (1s) | WiFi OK, MQTT desconectado |
| 🟢 Encendido fijo | WiFi + MQTT conectados |

---


```

### Estructura main.cpp (Módulos)
```
main.cpp
├── Configuración (pines, umbrales, timing)
├── Secrets (include/secrets.h)
├── Machine States (SEGURO/AUTO/MANUAL/ALARMA)
├── Sensor MQ2 (ADC + validación)
├── Alarm Logic (umbral, histéresis, contador)
├── Actuators (Buzzer PWM, LED WiFi patterns)
├── OLED Driver (buffer 5x7, I2C SSD1306)
├── Serial Comms (debug + comandos locales)
├── WiFi Manager (auto-reconnect, 10s retry)
├── MQTT Client (PubSubClient, 5s retry, QoS 0)
├── Telemetry (ArduinoJson, JSON 512 bytes)
└── Loop Principal (cooperative scheduler, millis())
``

---
```


---

## 📄 Licencia

Proyecto educativo - IU Digital de Antioquia - Internet de las Cosas
Uso académico únicamente.

---

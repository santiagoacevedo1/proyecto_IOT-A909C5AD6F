#ifndef SECRETS_H
#define SECRETS_H


// Para simular en Wokwi usa la red virtual "Wokwi-GUEST" (abierta, sin contraseña)

#define WIFI_SSID "Wokwi-GUEST"
#define WIFI_PASSWORD ""


#define MQTT_BROKER "broker.wokwi.com"
#define MQTT_PORT 1883
#define MQTT_USER ""
#define MQTT_PASS ""




#define MQTT_TOPIC_TELEMETRIA "iot/a909c5ad6f/telemetria"
#define MQTT_TOPIC_CONTROL    "iot/a909c5ad6f/control"


#define MQTT_CLIENT_ID "esp32-gas-" 

#endif 


#include <Arduino.h>
#include <Wire.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

#include "secrets.h"




static const char *ID_PROYECTO = "IOT-A909C5AD6F";


static const int UMBRAL_PRINCIPAL_ADC = 1100;
static const unsigned long INTERVALO_MUESTREO_MS = 1550;
static const uint8_t LECTURAS_CONSECUTIVAS_CONFIRMACION = 5;
static const int MARGEN_RETORNO_ADC = 6;
static const int UMBRAL_RETORNO_ADC = UMBRAL_PRINCIPAL_ADC - MARGEN_RETORNO_ADC; // 1094

static const int ADC_VALOR_MINIMO = 0;
static const int ADC_VALOR_MAXIMO = 4095;


static const uint8_t PIN_SENSOR_GAS_MQ2 = 34;
static const uint8_t PIN_BUZZER = 25;
static const uint8_t PIN_OLED_SDA = 21;
static const uint8_t PIN_OLED_SCL = 22;
static const uint8_t PIN_LED_WIFI = 27;


static const uint8_t OLED_DIRECCION_I2C = 0x3C;
static const uint8_t OLED_ANCHO_PX = 128;
static const uint8_t OLED_PAGINAS = 8;


static const unsigned long DURACION_PRUEBA_BUZZER_MS = 200;
static const uint16_t FRECUENCIA_ALARMA_HZ = 2000;

// --- Telemetría ---
static const char *NOMBRE_VARIABLE = "nivel_gas_MQ2";
static const char *UNIDAD_VARIABLE = "ADC";


static const unsigned long WIFI_REINTENTO_MS = 10000;
static const unsigned long MQTT_REINTENTO_MS = 5000;
static const unsigned long LED_PARPADEO_RAPIDO_MS = 100;
static const unsigned long LED_PARPADEO_LENTO_MS = 1000;

// aca logica de la maquina de estados

enum ModoSistema {
  MODO_SEGURO,      
  MODO_AUTO,        
  MODO_MANUAL       
};

enum EstadoAlarma {
  ALARMA_INACTIVA,
  ALARMA_ACTIVA
};

ModoSistema modoActual = MODO_SEGURO;
EstadoAlarma estadoAlarma = ALARMA_INACTIVA;


uint8_t lecturasEnRiesgoConsecutivas = 0;
int ultimaLecturaValida = 0;
bool ultimaLecturaEsValida = false;

// Timing  del proceso que no es bloqueante
unsigned long ultimoMuestreo = 0;
unsigned long ultimoIntentoWiFi = 0;
unsigned long ultimoIntentoMQTT = 0;
unsigned long ultimoParpadeoLED = 0;

// WiFi/MQTT
WiFiClient espClient;
PubSubClient mqttClient(espClient);
bool wifiConectado = false;
bool mqttConectado = false;

// LED WiFi
bool ledEstado = false;
unsigned long ledIntervaloActual = LED_PARPADEO_RAPIDO_MS;

// OLED
uint8_t oledBuffer[OLED_PAGINAS * OLED_ANCHO_PX];
uint8_t cursorColPx = 0;
uint8_t cursorPagina = 0;

// aca configuracion matriz de la pantalla en 5x7
struct GlifoFuente { char caracter; uint8_t columnas[5]; };

const GlifoFuente FUENTE_5X7[] = {
  {' ', {0x00, 0x00, 0x00, 0x00, 0x00}},
  {'!', {0x00, 0x00, 0x5F, 0x00, 0x00}},
  {'-', {0x08, 0x08, 0x08, 0x08, 0x08}},
  {'.', {0x00, 0x60, 0x60, 0x00, 0x00}},
  {'/', {0x20, 0x10, 0x08, 0x04, 0x02}},
  {':', {0x00, 0x36, 0x36, 0x00, 0x00}},
  {'0', {0x3E, 0x51, 0x49, 0x45, 0x3E}},
  {'1', {0x00, 0x42, 0x7F, 0x40, 0x00}},
  {'2', {0x42, 0x61, 0x51, 0x49, 0x46}},
  {'3', {0x21, 0x41, 0x45, 0x4B, 0x31}},
  {'4', {0x18, 0x14, 0x12, 0x7F, 0x10}},
  {'5', {0x27, 0x45, 0x45, 0x45, 0x39}},
  {'6', {0x3C, 0x4A, 0x49, 0x49, 0x30}},
  {'7', {0x01, 0x71, 0x09, 0x05, 0x03}},
  {'8', {0x36, 0x49, 0x49, 0x49, 0x36}},
  {'9', {0x06, 0x49, 0x49, 0x29, 0x1E}},
  {'A', {0x7E, 0x11, 0x11, 0x11, 0x7E}},
  {'C', {0x3E, 0x41, 0x41, 0x41, 0x22}},
  {'D', {0x7F, 0x41, 0x41, 0x22, 0x1C}},
  {'E', {0x7F, 0x49, 0x49, 0x49, 0x41}},
  {'F', {0x7F, 0x09, 0x09, 0x09, 0x01}},
  {'G', {0x3E, 0x41, 0x49, 0x49, 0x7A}},
  {'I', {0x00, 0x41, 0x7F, 0x41, 0x00}},
  {'L', {0x7F, 0x40, 0x40, 0x40, 0x40}},
  {'M', {0x7F, 0x02, 0x0C, 0x02, 0x7F}},
  {'N', {0x7F, 0x04, 0x08, 0x10, 0x7F}},
  {'O', {0x3E, 0x41, 0x41, 0x41, 0x3E}},
  {'P', {0x7F, 0x09, 0x09, 0x09, 0x06}},
  {'R', {0x7F, 0x09, 0x19, 0x29, 0x46}},
  {'S', {0x46, 0x49, 0x49, 0x49, 0x31}},
  {'T', {0x01, 0x01, 0x7F, 0x01, 0x01}},
  {'U', {0x3F, 0x40, 0x40, 0x40, 0x3F}},
  {'V', {0x1F, 0x20, 0x40, 0x20, 0x1F}},
  {'Y', {0x07, 0x08, 0x70, 0x08, 0x07}},
  {'_', {0x00, 0x00, 0x00, 0x00, 0x7F}},
  {'#', {0x14, 0x7F, 0x14, 0x7F, 0x14}},
  {'@', {0x3E, 0x41, 0x5D, 0x55, 0x1E}},
  {'?', {0x02, 0x01, 0x51, 0x09, 0x06}},
  {'>', {0x08, 0x14, 0x22, 0x41, 0x00}},
  {'<', {0x00, 0x41, 0x22, 0x14, 0x08}},
};
const uint8_t CANTIDAD_GLIFOS = sizeof(FUENTE_5X7) / sizeof(FUENTE_5X7[0]);



// Inicia
void inicializarPines();
void inicializarSerial();
void inicializarOLED();
void inicializarWiFi();
void inicializarMQTT();

// aca loop principal, recordar volver acá
void loopPrincipal();
void gestionarMuestreo();
void gestionarWiFi();
void gestionarMQTT();
void gestionarLEDWiFi();
void gestionarComandosSerial();

// Sensor y Alarma
int leerSensorGas();
bool lecturaEsValida(int lectura);
void actualizarLogicaAlarma(int lectura, bool valida);
void aplicarEstadoSeguro();

// Actuadores
void controlarBuzzer();
void pruebaBuzzerInicial();

// OLED
void oledComando(uint8_t cmd);
void oledInicializar();
void oledLimpiarBuffer();
void oledMostrar();
void oledSetCursor(uint8_t col, uint8_t pag);
const uint8_t* buscarGlifo(char c);
void oledDibujarCaracter(char c);
void oledImprimir(const char* texto);
void oledImprimirLinea(const char* texto);
void actualizarInterfazOLED();

// Comunicación
void publicarTelemetriaMQTT();
void publicarSerial();
void callbackMQTT(char* topic, byte* payload, unsigned int length);
void procesarComando(const char* comando);
void reconectarWiFi();
void reconectarMQTT();
String generarClientID();

// Utilidades
unsigned long millisSeguro();

void setup() {
  inicializarPines();
  inicializarSerial();
oledInicializar();

  // aca la pantalla de inicio
  oledLimpiarBuffer();
  oledSetCursor(0, 0);
  oledImprimirLinea("PROYECTO");
  oledSetCursor(0, 1);
  oledImprimirLinea(ID_PROYECTO);
  oledSetCursor(0, 2);
  oledImprimirLinea("INICIANDO...");
  oledMostrar();

  pruebaBuzzerInicial();
  aplicarEstadoSeguro();

  inicializarWiFi();
  inicializarMQTT();

  oledLimpiarBuffer();
  oledSetCursor(0, 0);
  oledImprimirLinea("LISTO");
  oledMostrar();

  Serial.println(F(">>> Sistema listo. Loop principal iniciado."));
}

// EL LOOP QUE NO ES BLOQUEANTE

void loop() {
  loopPrincipal();
}

void loopPrincipal() {
  unsigned long ahora = millisSeguro();

  // ACA GESTIONO CONEXION WIFI YY QUEDA DE MANERA AUTOMÁTICA
  gestionarWiFi();

  
  gestionarMQTT();

  
  gestionarLEDWiFi();

  
  if (ahora - ultimoMuestreo >= INTERVALO_MUESTREO_MS) {
    gestionarMuestreo();
    ultimoMuestreo = ahora;
  }

 
  controlarBuzzer();

  
  gestionarComandosSerial();

  actualizarInterfazOLED();

  
  delay(1);
}

// ALARMA ---- PIEZOELECTRICO

void gestionarMuestreo() {
  int lectura = leerSensorGas();
  bool valida = lecturaEsValida(lectura);

  ultimaLecturaValida = lectura;
  ultimaLecturaEsValida = valida;

  
  if (modoActual == MODO_AUTO) {
    actualizarLogicaAlarma(lectura, valida);
  }

  // aca desarrollo de publicacion de telemetria
  publicarTelemetriaMQTT();

//**************************************** */
  // profe, ayuda acá por favor si ve un error, pora favor
  publicarSerial();
}

//**************************************** */
void actualizarLogicaAlarma(int lectura, bool valida) {
  if (!valida) return;

  if (lectura >= UMBRAL_PRINCIPAL_ADC) {
    if (lecturasEnRiesgoConsecutivas < 255) {
      lecturasEnRiesgoConsecutivas++;
    }
    if (lecturasEnRiesgoConsecutivas >= LECTURAS_CONSECUTIVAS_CONFIRMACION) {
      estadoAlarma = ALARMA_ACTIVA;
    }
  } else if (lectura <= UMBRAL_RETORNO_ADC) {
    lecturasEnRiesgoConsecutivas = 0;
    estadoAlarma = ALARMA_INACTIVA;
  }
}

void controlarBuzzer() {
  if (estadoAlarma == ALARMA_ACTIVA && modoActual == MODO_AUTO) {
    tone(PIN_BUZZER, FRECUENCIA_ALARMA_HZ);
  } else {
    noTone(PIN_BUZZER);
  }
}



void inicializarWiFi() {
  Serial.print(F("Conectando WiFi..."));

  
  WiFi.mode(WIFI_STA);

  if (strlen(WIFI_SSID) > 0) {
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  } else {
   
    WiFi.begin();
  }

  ultimoIntentoWiFi = millisSeguro();
}

void gestionarWiFi() {
  unsigned long ahora = millisSeguro();

  if (WiFi.status() == WL_CONNECTED) {
    if (!wifiConectado) {
      wifiConectado = true;
      Serial.println(F("\nWiFi CONECTADO"));
      Serial.print(F("IP: ")); Serial.println(WiFi.localIP());
      Serial.print(F("RSSI: ")); Serial.print(WiFi.RSSI()); Serial.println(F(" dBm"));
      
      ultimoIntentoMQTT = 0;
    }
  } else {
    if (wifiConectado) {
      wifiConectado = false;
      mqttConectado = false; // MQTT se cae si WiFi se cae
      Serial.println(F("\nWiFi DESCONECTADO"));
    }
   
    if (ahora - ultimoIntentoWiFi >= WIFI_REINTENTO_MS) {
      Serial.print(F("Reintentando WiFi..."));
      WiFi.reconnect();
      ultimoIntentoWiFi = ahora;
    }
  }
}

void inicializarMQTT() {
  mqttClient.setServer(MQTT_BROKER, MQTT_PORT);
  mqttClient.setCallback(callbackMQTT);
  mqttClient.setKeepAlive(60);
  ultimoIntentoMQTT = 0;
}

void gestionarMQTT() {
  if (!wifiConectado) {
    if (mqttConectado) {
      mqttConectado = false;
      Serial.println(F("MQTT: desconectado (sin WiFi)"));
    }
    return;
  }

  if (!mqttClient.connected()) {
    unsigned long ahora = millisSeguro();
    if (ahora - ultimoIntentoMQTT >= MQTT_REINTENTO_MS) {
      reconectarMQTT();
      ultimoIntentoMQTT = ahora;
    }
    return;
  }

  if (!mqttConectado) {
    mqttConectado = true;
    Serial.println(F("MQTT CONECTADO"));
    
    mqttClient.subscribe(MQTT_TOPIC_CONTROL);
    Serial.print(F("Suscrito a: ")); Serial.println(MQTT_TOPIC_CONTROL);
  }

  
  mqttClient.loop();
}

void reconectarMQTT() {
  String clientId = generarClientID();
  Serial.print(F("MQTT conectando como ")); Serial.print(clientId); Serial.print(F("... "));

  bool connected = false;
  if (strlen(MQTT_USER) > 0) {
    connected = mqttClient.connect(clientId.c_str(), MQTT_USER, MQTT_PASS);
  } else {
    connected = mqttClient.connect(clientId.c_str());
  }

  if (connected) {
    Serial.println(F("OK"));
    mqttClient.subscribe(MQTT_TOPIC_CONTROL);
  } else {
    Serial.print(F("FALLÓ, rc=")); Serial.println(mqttClient.state());
  }
}

String generarClientID() {
  uint8_t mac[6];
  WiFi.macAddress(mac);
  char buf[32];
  snprintf(buf, sizeof(buf), "%s%02X%02X%02X", MQTT_CLIENT_ID, mac[3], mac[4], mac[5]);
  return String(buf);
}

void callbackMQTT(char* topic, byte* payload, unsigned int length) {
  
  char mensaje[length + 1];
  memcpy(mensaje, payload, length);
  mensaje[length] = '\0';

  Serial.print(F("MQTT RX [")); Serial.print(topic); Serial.print(F("]: ")); Serial.println(mensaje);

  if (strcmp(topic, MQTT_TOPIC_CONTROL) == 0) {
    procesarComando(mensaje);
  }
}

void procesarComando(const char* comando) {
  StaticJsonDocument<128> doc;
  DeserializationError err = deserializeJson(doc, comando);

  if (err) {
    Serial.print(F("JSON inválido: ")); Serial.println(err.c_str());
    return;
  }

  const char* cmd = doc["comando"];
  if (!cmd) {
    Serial.println(F("Comando sin campo 'comando'"));
    return;
  }

  Serial.print(F("Comando recibido: ")); Serial.println(cmd);

  if (strcmp(cmd, "AUTO") == 0) {
    modoActual = MODO_AUTO;
    Serial.println(F(">>> Modo: AUTO"));
  } else if (strcmp(cmd, "MANUAL") == 0) {
    modoActual = MODO_MANUAL;
    estadoAlarma = ALARMA_INACTIVA;
    lecturasEnRiesgoConsecutivas = 0;
    Serial.println(F(">>> Modo: MANUAL (alarma desactivada)"));
  } else if (strcmp(cmd, "RESET") == 0) {
    aplicarEstadoSeguro();
    modoActual = MODO_AUTO; // Volver a auto tras reset
    Serial.println(F(">>> RESET: Estado seguro + modo AUTO"));
  } else {
    Serial.print(F("Comando desconocido: ")); Serial.println(cmd);
  }
}



void publicarTelemetriaMQTT() {
  if (!mqttConectado) return;

  StaticJsonDocument<512> doc;

  doc["proyecto"] = ID_PROYECTO;
  doc["timestamp_ms"] = millisSeguro();

  JsonObject sensor = doc.createNestedObject("sensor");
  sensor["tipo"] = "MQ2";
  sensor["nivel_gas"] = ultimaLecturaValida;
  sensor["unidad"] = UNIDAD_VARIABLE;
  sensor["valido"] = ultimaLecturaEsValida;

  doc["estado"] = (estadoAlarma == ALARMA_ACTIVA) ? "ALARMA" : "SEGURO";

  const char* modoStr = (modoActual == MODO_AUTO) ? "AUTO" :
                        (modoActual == MODO_MANUAL) ? "MANUAL" : "SEGURO";
  doc["modo"] = modoStr;

  JsonObject alarma = doc.createNestedObject("alarma");
  alarma["confirmada"] = (estadoAlarma == ALARMA_ACTIVA);
  alarma["lecturas_consecutivas"] = lecturasEnRiesgoConsecutivas;
  alarma["umbral"] = UMBRAL_PRINCIPAL_ADC;
  alarma["histeresis"] = MARGEN_RETORNO_ADC;

  char buffer[512];
  size_t n = serializeJson(doc, buffer, sizeof(buffer));

  if (n > 0) {
    bool ok = mqttClient.publish(MQTT_TOPIC_TELEMETRIA, buffer, false);
    if (!ok) {
      Serial.println(F("MQTT publish falló"));
    }
  }
}

// aca el led para el wifi

void gestionarLEDWiFi() {
  unsigned long ahora = millisSeguro();

  // Determinar patrón según estado
  unsigned long nuevoIntervalo;
  bool nuevoEstadoFijo = false;

  if (!wifiConectado) {
    // queda apagado fijo
    nuevoIntervalo = 0;
    nuevoEstadoFijo = false;
  } else if (!mqttConectado) {
    // parpadea muy lento
    nuevoIntervalo = LED_PARPADEO_LENTO_MS;
  } else {
    // queda encendido de manera fija
    nuevoIntervalo = 0;
    nuevoEstadoFijo = true;
  }

  // Cambia el patrón cuando conecta
  if (nuevoIntervalo != ledIntervaloActual || nuevoEstadoFijo != ledEstado) {
    ledIntervaloActual = nuevoIntervalo;
    if (nuevoEstadoFijo) {
      digitalWrite(PIN_LED_WIFI, HIGH);
      ledEstado = true;
      return;
    }
    if (nuevoIntervalo == 0) {
      digitalWrite(PIN_LED_WIFI, LOW);
      ledEstado = false;
      return;
    }
  }

 
  if (ledIntervaloActual > 0 && ahora - ultimoParpadeoLED >= ledIntervaloActual) {
    ledEstado = !ledEstado;
    digitalWrite(PIN_LED_WIFI, ledEstado ? HIGH : LOW);
    ultimoParpadeoLED = ahora;
  }
}



void gestionarComandosSerial() {
  if (!Serial.available()) return;

  String linea = Serial.readStringUntil('\n');
  linea.trim();
  linea.toUpperCase();

  if (linea.length() == 0) return;

  Serial.print(F("Serial CMD: ")); Serial.println(linea);

  if (linea == "AUTO" || linea == "MANUAL" || linea == "RESET" || linea == "STATUS") {
   
    StaticJsonDocument<64> doc;
    doc["comando"] = linea;
    char buffer[64];
    serializeJson(doc, buffer);
    procesarComando(buffer);
  } else if (linea == "HELP" || linea == "?") {
    Serial.println(F("Comandos: AUTO, MANUAL, RESET, STATUS, HELP"));
  } else {
    Serial.println(F("Comando no reconocido. Use HELP"));
  }
}



void publicarSerial() {
  Serial.print(F("var=")); Serial.print(NOMBRE_VARIABLE);
  Serial.print(F(" | val=")); Serial.print(ultimaLecturaValida);
  Serial.print(F(" | unit=")); Serial.print(UNIDAD_VARIABLE);
  Serial.print(F(" | valid=")); Serial.print(ultimaLecturaEsValida ? F("si") : F("no"));
  Serial.print(F(" | riesgo=")); Serial.print(lecturasEnRiesgoConsecutivas);
  Serial.print(F(" | estado=")); Serial.print(estadoAlarma == ALARMA_ACTIVA ? F("ALARMA") : F("SEGURO"));
  Serial.print(F(" | modo="));
  switch (modoActual) {
    case MODO_SEGURO: Serial.print(F("SEGURO")); break;
    case MODO_AUTO: Serial.print(F("AUTO")); break;
    case MODO_MANUAL: Serial.print(F("MANUAL")); break;
  }
  Serial.print(F(" | wifi=")); Serial.print(wifiConectado ? F("OK") : F("NC"));
  Serial.print(F(" | mqtt=")); Serial.println(mqttConectado ? F("OK") : F("NC"));
}



void inicializarPines() {
  analogReadResolution(12);
  pinMode(PIN_BUZZER, OUTPUT);
  pinMode(PIN_LED_WIFI, OUTPUT);
  digitalWrite(PIN_LED_WIFI, LOW);
  digitalWrite(PIN_BUZZER, LOW);
}

void inicializarSerial() {
  Serial.begin(115200);
  delay(200);
  Serial.println();
  Serial.println(F("******************************"));
  Serial.print(F("Proyecto IoT: ")); Serial.println(ID_PROYECTO);
  Serial.println(F("Actividad 2: WiFi + MQTT + Telemetria"));
  Serial.println(F("Advertencia: Prototipo educativo"));
  Serial.println(F("******************************"));
}

void pruebaBuzzerInicial() {
  Serial.println(F("Prueba buzzer..."));
  tone(PIN_BUZZER, FRECUENCIA_ALARMA_HZ);
  delay(DURACION_PRUEBA_BUZZER_MS);
  noTone(PIN_BUZZER);
  Serial.println(F("Buzzer OK"));
}

void aplicarEstadoSeguro() {
  noTone(PIN_BUZZER);
  estadoAlarma = ALARMA_INACTIVA;
  lecturasEnRiesgoConsecutivas = 0;
  modoActual = MODO_SEGURO;
  Serial.println(F("Estado seguro aplicado"));
}

// igualmente, el mq2

int leerSensorGas() {
  return analogRead(PIN_SENSOR_GAS_MQ2);
}

bool lecturaEsValida(int lectura) {
  return (lectura >= ADC_VALOR_MINIMO && lectura <= ADC_VALOR_MAXIMO);
}

// conexion a la matriz 5x7 de pantalla

void oledComando(uint8_t cmd) {
  Wire.beginTransmission(OLED_DIRECCION_I2C);
  Wire.write(0x00);
  Wire.write(cmd);
  Wire.endTransmission();
}

void oledInicializar() {
  Wire.begin(PIN_OLED_SDA, PIN_OLED_SCL);
  Wire.setClock(400000);
  delay(50);

  oledComando(0xAE); // Display OFF
  oledComando(0xD5); oledComando(0x80);
  oledComando(0xA8); oledComando(0x3F);
  oledComando(0xD3); oledComando(0x00);
  oledComando(0x40);
  oledComando(0x8D); oledComando(0x14);
  oledComando(0x20); oledComando(0x02);
  oledComando(0xA1);
  oledComando(0xC8);
  oledComando(0xDA); oledComando(0x12);
  oledComando(0x81); oledComando(0xCF);
  oledComando(0xD9); oledComando(0xF1);
  oledComando(0xDB); oledComando(0x40);
  oledComando(0xA4);
  oledComando(0xA6);
  oledComando(0xAF); // Display ON
}

void oledLimpiarBuffer() {
  memset(oledBuffer, 0x00, sizeof(oledBuffer));
  cursorColPx = 0;
  cursorPagina = 0;
}

void oledMostrar() {
  for (uint8_t pagina = 0; pagina < OLED_PAGINAS; pagina++) {
    oledComando(0xB0 + pagina);
    oledComando(0x00);
    oledComando(0x10);

    const uint8_t TAM_BLOQUE = 16;
    for (uint8_t inicio = 0; inicio < OLED_ANCHO_PX; inicio += TAM_BLOQUE) {
      Wire.beginTransmission(OLED_DIRECCION_I2C);
      Wire.write(0x40);
      for (uint8_t i = 0; i < TAM_BLOQUE; i++) {
        Wire.write(oledBuffer[pagina * OLED_ANCHO_PX + inicio + i]);
      }
      Wire.endTransmission();
    }
  }
}

void oledSetCursor(uint8_t col, uint8_t pag) {
  cursorColPx = col;
  cursorPagina = pag;
}

const uint8_t* buscarGlifo(char c) {
  for (uint8_t i = 0; i < CANTIDAD_GLIFOS; i++) {
    if (FUENTE_5X7[i].caracter == c) {
      return FUENTE_5X7[i].columnas;
    }
  }
  return nullptr;
}

void oledDibujarCaracter(char c) {
  if (cursorColPx + 6 > OLED_ANCHO_PX || cursorPagina >= OLED_PAGINAS) return;

  const uint8_t* glifo = buscarGlifo(c);
  uint16_t base = (uint16_t)cursorPagina * OLED_ANCHO_PX + cursorColPx;

  for (uint8_t i = 0; i < 5; i++) {
    oledBuffer[base + i] = glifo ? glifo[i] : 0x00;
  }
  oledBuffer[base + 5] = 0x00; 
  cursorColPx += 6;
}

void oledImprimir(const char* texto) {
  for (const char* p = texto; *p != '\0'; p++) {
    oledDibujarCaracter(*p);
  }
}

void oledImprimirLinea(const char* texto) {
  oledImprimir(texto);
  cursorPagina++;
  cursorColPx = 0;
}

void actualizarInterfazOLED() {
  oledLimpiarBuffer();

  
  oledSetCursor(0, 0);
  oledImprimirLinea(ID_PROYECTO);

  char lineaGas[20];
  snprintf(lineaGas, sizeof(lineaGas), "GAS: %d", ultimaLecturaValida);
  oledSetCursor(0, 1);
  oledImprimirLinea(lineaGas);

  
  oledSetCursor(0, 2);
  oledImprimirLinea(ultimaLecturaEsValida ? "VALIDA: SI" : "VALIDA: NO");

  
  char lineaModo[20];
  const char* modoStr = (modoActual == MODO_AUTO) ? "AUTO" :
                        (modoActual == MODO_MANUAL) ? "MANUAL" : "SEGURO";
  snprintf(lineaModo, sizeof(lineaModo), "MODO: %s", modoStr);
  oledSetCursor(0, 3);
  oledImprimirLinea(lineaModo);


  oledSetCursor(0, 4);
  if (!ultimaLecturaEsValida) {
    oledImprimirLinea("-- DATO INVALIDO --");
  } else if (estadoAlarma == ALARMA_ACTIVA && modoActual == MODO_AUTO) {
    oledImprimirLinea(">>> ALARMA! <<<");
  } else {
    oledImprimirLinea("SEGURO");
  }

  
  char lineaCont[20];
  snprintf(lineaCont, sizeof(lineaCont), "CONSEC: %d/5", lecturasEnRiesgoConsecutivas);
  oledSetCursor(0, 5);
  oledImprimirLinea(lineaCont);

  
  char lineaNet[20];
  snprintf(lineaNet, sizeof(lineaNet), "W:%s M:%s",
           wifiConectado ? "OK" : "--",
           mqttConectado ? "OK" : "--");
  oledSetCursor(0, 6);
  oledImprimirLinea(lineaNet);

  char lineaUp[20];
  unsigned long seg = millisSeguro() / 1000;
  snprintf(lineaUp, sizeof(lineaUp), "UP: %lus", seg);
  oledSetCursor(0, 7);
  oledImprimirLinea(lineaUp);

  oledMostrar();
}



unsigned long millisSeguro() {
  return millis(); // En ESP32 Arduino, millis() maneja overflow internamente
}
/*
 * Proyecto:   IOT-A909C5AD6F
 * Cuarto técnico - detección simulada de nivel de gas
 */

#include <Arduino.h>
#include <Wire.h>


static const char *ID_PROYECTO = "IOT-A909C5AD6F";
static const int UMBRAL_PRINCIPAL_ADC = 1100;                 // este es el umbral que marco para la alarma
static const unsigned long INTERVALO_MUESTREO_MS = 1550;      // ms
static const uint8_t LECTURAS_CONSECUTIVAS_CONFIRMACION = 5;  // numero de lecturas que marco para el piezoeléctrico
static const int MARGEN_RETORNO_ADC = 6;                      // unidades ADC
static const int UMBRAL_RETORNO_ADC = UMBRAL_PRINCIPAL_ADC - MARGEN_RETORNO_ADC; // 1094 indicado


static const int ADC_VALOR_MINIMO = 0;
static const int ADC_VALOR_MAXIMO = 4095;

static const uint8_t PIN_SENSOR_GAS_MQ2 = 34; 
static const uint8_t PIN_BUZZER = 25;         
static const uint8_t PIN_OLED_SDA = 21;       
static const uint8_t PIN_OLED_SCL = 22;       

// Aquí pongo parámetros locales de la interfaz de la OLED
static const uint8_t OLED_DIRECCION_I2C = 0x3C;
static const uint8_t OLED_ANCHO_PX = 128;
static const uint8_t OLED_PAGINAS = 8; // 64 pixeles de alto / 8 pixels por pagina

static const unsigned long DURACION_PRUEBA_BUZZER_MS = 200;


static const char *NOMBRE_VARIABLE = "nivel_gas_MQ2";
static const char *UNIDAD_VARIABLE = "ADC";


// lógica de la alarma 

uint8_t lecturasEnRiesgoConsecutivas = 0; // contador para la alarma
bool alarmaConfirmada = false;            

uint8_t oledBuffer[OLED_PAGINAS * OLED_ANCHO_PX];

uint8_t cursorColPx = 0;
uint8_t cursorPagina = 0;

// Uso una fuente de matriz de 5x7
// OJO ACA cada caracter son 5 bytes

struct GlifoFuente {
  char caracter;
  uint8_t columnas[5];
};

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
};
const uint8_t CANTIDAD_GLIFOS = sizeof(FUENTE_5X7) / sizeof(FUENTE_5X7[0]);

const uint8_t *buscarGlifo(char c) {
  for (uint8_t i = 0; i < CANTIDAD_GLIFOS; i++) {
    if (FUENTE_5X7[i].caracter == c) {
      return FUENTE_5X7[i].columnas;
    }
  }
  return nullptr; // este caracter me tocó dibujarlo en blanco porque no es soportado
}

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

  oledComando(0xAE); // pantalla off
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
  oledComando(0xAF);                    // pantalla on
}

void oledLimpiarBuffer() {
  memset(oledBuffer, 0x00, sizeof(oledBuffer));
  cursorColPx = 0;
  cursorPagina = 0;
}

void oledMostrar() {
  for (uint8_t pagina = 0; pagina < OLED_PAGINAS; pagina++) {
    oledComando(0xB0 + pagina); // concateno la pagina de inicio
    oledComando(0x00);          
    oledComando(0x10);          

    
    
    const uint8_t TAMANO_BLOQUE = 16;
    for (uint8_t inicio = 0; inicio < OLED_ANCHO_PX; inicio += TAMANO_BLOQUE) {
      Wire.beginTransmission(OLED_DIRECCION_I2C);
      Wire.write(0x40); 
      for (uint8_t i = 0; i < TAMANO_BLOQUE; i++) {
        Wire.write(oledBuffer[pagina * OLED_ANCHO_PX + inicio + i]);
      }
      Wire.endTransmission();
    }
  }
}

void oledSetCursor(uint8_t colPx, uint8_t pagina) {
  cursorColPx = colPx;
  cursorPagina = pagina;
}

void oledDibujarCaracter(char c) {
  if (cursorColPx + 6 > OLED_ANCHO_PX || cursorPagina >= OLED_PAGINAS) {
    return; 
  }
  const uint8_t *glifo = buscarGlifo(c);
  uint16_t base = (uint16_t)cursorPagina * OLED_ANCHO_PX + cursorColPx;
  for (uint8_t i = 0; i < 5; i++) {
    oledBuffer[base + i] = glifo ? glifo[i] : 0x00;
  }
  oledBuffer[base + 5] = 0x00; 
  cursorColPx += 6;
}

void oledImprimir(const char *texto) {
  for (const char *p = texto; *p != '\0'; p++) {
    oledDibujarCaracter(*p);
  }
}

void oledImprimirLinea(const char *texto) {
  oledImprimir(texto);
  cursorPagina++;
  cursorColPx = 0;
}

//lo anterior deje marcada la configuracion de pantalla por guia de IA como indica el profe y ejemplos de librerias

// A continuacion lógica del proyecto

void inicializarSerial();
void inicializarSensorYActuador();
void probarActuadorDeFormaControlada();
void aplicarEstadoSeguro();
int leerSensorGas();
bool lecturaEsValida(int lecturaCruda);
void actualizarLogicaDeAlarma(int lectura, bool valida);
void publicarSerial(int lectura, bool valida);
void actualizarInterfazLocal(int lectura, bool valida);


void setup() {
  
  inicializarSerial();

  
  inicializarSensorYActuador();
  oledInicializar();

  oledLimpiarBuffer();
  oledSetCursor(0, 0);
  oledImprimirLinea("PROYECTO");
  oledSetCursor(0, 1);
  oledImprimirLinea(ID_PROYECTO);
  oledSetCursor(0, 2);
  oledImprimirLinea("INICIANDO");
  oledMostrar();
  delay(500);

  
  probarActuadorDeFormaControlada();
  aplicarEstadoSeguro();
}


void loop() {
  
  int lecturaCruda = leerSensorGas();
  bool valida = lecturaEsValida(lecturaCruda);
  actualizarLogicaDeAlarma(lecturaCruda, valida);
  publicarSerial(lecturaCruda, valida);
  actualizarInterfazLocal(lecturaCruda, valida);
   if (alarmaConfirmada) {
    tone(PIN_BUZZER, 2000);
  } else {
    noTone(PIN_BUZZER);
  }
  delay(INTERVALO_MUESTREO_MS);
}


void inicializarSerial() {
  Serial.begin(115200);
  delay(200); 
  Serial.println();
  Serial.println(F("******************"));
  Serial.print(F("Proyecto IoT: "));
  Serial.println(ID_PROYECTO);
  Serial.println(F("Escenario: cuarto tecnico - deteccion simulada de gas"));
  Serial.println(F("Advertencia: prototipo educativo, NO es un instrumento certificado."));
  Serial.println(F("******************"));
}

void inicializarSensorYActuador() {
  
  analogReadResolution(12); 
  pinMode(PIN_BUZZER, OUTPUT);
}

void probarActuadorDeFormaControlada() {
  Serial.println(F("Prueba controlada del actuador (buzzer)..."));
  tone(PIN_BUZZER, 2000);
  delay(DURACION_PRUEBA_BUZZER_MS);
  noTone(PIN_BUZZER);
  Serial.println(F("Prueba de actuador finalizada."));
}

void aplicarEstadoSeguro() {
  noTone(PIN_BUZZER);
  alarmaConfirmada = false;
  lecturasEnRiesgoConsecutivas = 0;
  Serial.println(F("Estado seguro aplicado."));
}

int leerSensorGas() {
  return analogRead(PIN_SENSOR_GAS_MQ2);
}

bool lecturaEsValida(int lecturaCruda) {
  return (lecturaCruda >= ADC_VALOR_MINIMO && lecturaCruda <= ADC_VALOR_MAXIMO);
}

void actualizarLogicaDeAlarma(int lectura, bool valida) {
  if (!valida) {
    return;
  }

  if (lectura >= UMBRAL_PRINCIPAL_ADC) {
    
    if (lecturasEnRiesgoConsecutivas < 255) {
      lecturasEnRiesgoConsecutivas++;
    }
    if (lecturasEnRiesgoConsecutivas >= LECTURAS_CONSECUTIVAS_CONFIRMACION) {
      alarmaConfirmada = true;
    }
  } else if (lectura <= UMBRAL_RETORNO_ADC) {
    
    lecturasEnRiesgoConsecutivas = 0;
    alarmaConfirmada = false;
  }
  
}

void publicarSerial(int lectura, bool valida) {
  Serial.print(F("variable="));
  Serial.print(NOMBRE_VARIABLE);
  Serial.print(F(" | valor="));
  Serial.print(lectura);
  Serial.print(F(" | unidad="));
  Serial.print(UNIDAD_VARIABLE);
  Serial.print(F(" | valida="));
  Serial.print(valida ? F("si") : F("no"));
  Serial.print(F(" | lecturas_en_riesgo="));
  Serial.print(lecturasEnRiesgoConsecutivas);
  Serial.print(F(" | estado="));
  Serial.println(alarmaConfirmada ? F("ALARMA") : F("SEGURO"));
}

void actualizarInterfazLocal(int lectura, bool valida) {
  oledLimpiarBuffer();

  oledSetCursor(0, 0);
  oledImprimirLinea(ID_PROYECTO);

  char lineaGas[20];
  snprintf(lineaGas, sizeof(lineaGas), "GAS: %d", lectura);
  oledSetCursor(0, 1);
  oledImprimirLinea(lineaGas);

  oledSetCursor(0, 2);
  oledImprimirLinea(valida ? "VALIDA: SI" : "VALIDA: NO");

  oledSetCursor(0, 4);
  if (!valida) {
    oledImprimirLinea("--DATO--");
  } else if (alarmaConfirmada) {
    oledImprimirLinea("ALARMA!");
  } else {
    oledImprimirLinea("SEGURO");
  }

  oledMostrar();
}

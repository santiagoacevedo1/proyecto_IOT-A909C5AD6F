# Ficha de insumos individuales — Actividad 1 

// Van incluidos los de la ACT 1

## 1.1. Datos del escenario

- **Contexto:** un cuarto técnico.
- **Propósito:** advertir una condición simulada de riesgo, sin presentarla como
  instrumento certificado.
- **Variable principal:** nivel analógico de gas.
- **Regla asignada:** activar la alarma cuando la lectura sea mayor o igual al umbral.
- **Identificador del proyecto:** IOT-A909C5AD6F

## 1.2. Componentes que me asignó Google Colab


| Rol | Componente | Identificador de Wokwi |
|---|---|---|
| Microcontrolador | ESP32 DevKit v1 | `wokwi-esp32-devkit-v1` |
| Sensor | Sensor de gas MQ2 - Analógica | `wokwi-gas-sensor` |
| Actuador | Buzzer piezoeléctrico | `wokwi-buzzer` |
| Interfaz local | Pantalla OLED SSD1306 | `board-ssd1306` |

## 1.3. Parámetros individuales

| Parámetro | Valor |
|---|---|
| Umbral principal | 1100 unidades ADC |
| Intervalo de muestreo | 1550 ms |
| Confirmación de alarma | 5 lecturas consecutivas |
| Margen de retorno | 6 unidades respecto al umbral |
| Estado seguro | apagado, excepto ante una alarma local confirmada |

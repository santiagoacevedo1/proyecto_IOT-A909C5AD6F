1. El comportamiento local debería ser: 
la pantalla OLED, con  una actualización cada 1.55 segundos
muestra específicamente 7 líneas de información con cada uno de los requerimientos: indentificador de proyecto, lectura del sensor MQ2, validez de la lectura, modo actual(auto, manual, seguro), estado de la alarma (seguro, alarma, inválido: en caso de que el sensor detecte lectura mínimas), un contador de lecturas consectivas 0/0, un estado conectividad wifi (wifi ok, MQTT ok), un tiempo de inactividad en segundos.
La actuación del piezo electrico sigue siendo la misma: un sonido inicial de prueba, se activa luego de 5 lecturas consecutivas (1100 unidades ADC).
Un Led Wifi con indicador visual en tiempo real: apagado fijo-- sin conexion
parpadeo rápido-- conectado a wifi
parpadeo lento--wifi conectado, MQTT desconectado
encedido fijo-- wifi y MQTT conectados

2. Se activa luego de 5 lecturas consecutivas, un equivalente temporal a uno 6.2 segundos, 1100 unidades ADC.

3. la Histeresis asignadas cumplen la función de, al bajar a un rango menor o igual a 1100 ADC, el contador de unidades se reinicia y vuelve a 0, la alarma se desctiva y el buzzer queda apagado

4. La información que se envía a la nube, e suna telemetría publica cada 1.55 segundos con información de las 7 líneas OLED, que recibe comando de AUTO, MANUAL, RESET. Tambíen recibe la información de la conexión wifi cada 10 segundos y del MQTT cada 5 segundos.

5. Dos dificultades que tuve fue:
  A. La integración de telemetría en el proyecto, ya que, com o cambié a sistema operativo Linux, debí hacer algunas modificaciones por comandos que me dificultaron la integración del proceso.
  B. la integración de conectividad wifi: por el mismo sistema operativo al que cambién, no diferenciaba y no tenía conocimiento de código de cuando podía ser simulado y cuando debía ser para conexión real de Hardware, por lo cual, no tenía claro si la integración de credenciales debía ser también pára el proceso simulado en wokwi.
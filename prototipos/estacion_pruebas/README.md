# Estación de campo de pruebas Arduino – TierraViva

Esta carpeta recoge las pruebas incrementales realizadas para construir la estación de campo del sistema **TierraViva**.

A diferencia del prototipo local basado en MQTT, esta parte del repositorio corresponde a la evolución de la estación de campo final: un nodo basado en **Arduino UNO**, sensores ambientales, sensor de humedad de suelo, comunicación 4G/LTE mediante el módulo **A7670E** y envío de telemetría a **ThingSpeak**.

El objetivo de esta carpeta no es presentar únicamente el código final, sino documentar el proceso técnico seguido hasta alcanzar una estación funcional. Por ello se conservan sketches intermedios, pruebas descartadas, ensayos de comunicación y documentación técnica asociada al módem LTE.

Los nombres originales de los sketches se han conservado para mantener la compatibilidad con Arduino IDE, ya que cada proyecto de Arduino requiere que la carpeta y el archivo `.ino` principal tengan el mismo nombre.



## Componentes probados

Las pruebas de esta carpeta cubren los siguientes bloques funcionales de la estación:

* Lectura de humedad de suelo mediante sensor capacitivo.
* Lectura de variables ambientales mediante BME/BMP280.
* Lectura de luminosidad mediante BH1750.
* Comunicación serie entre Arduino UNO y el módulo A7670E.
* Pruebas de comandos AT sobre el módem LTE.
* Registro en red móvil 4G/LTE.
* Envío de datos a ThingSpeak mediante HTTP.
* Integración progresiva de sensores, comunicaciones y lógica de estación.
* Optimización de memoria para ejecución en Arduino UNO.


## Índice de sketches

| Sketch                          | Función                                                           | Estado                         |
| ------------------------------- | ----------------------------------------------------------------- | ------------------------------ |
| `ArduinoUNO_A7670E_1er_test`    | Primera prueba de comunicación Arduino UNO ↔ A7670E               | Prueba inicial                 |
| `ArduinoUNO_A7670E`             | Prueba básica de integración entre Arduino UNO y el módulo A7670E | Prueba inicial                 |
| `UARTUNO_A7670E`                | Prueba mínima de comunicación UART entre Arduino y el módem       | Prueba inicial                 |
| `A7670E_SWSERIAL`               | Ensayo de comunicación con el A7670E usando SoftwareSerial        | Diagnóstico                    |
| `A7670E_115200b`                | Ensayo de comunicación con el módem a 115200 baudios              | Descartado / ajuste de baudios |
| `A7670E_9600b`                  | Prueba de comunicación con el módem a 9600 baudios                | Integrado                      |
| `A7670E_9600b_masespera`        | Variante con mayor tiempo de espera durante el arranque del módem | Prueba de robustez             |
| `A7670E_ComandosPrueba`         | Envío secuencial de comandos AT al A7670E                         | Diagnóstico                    |
| `PruebafinalCOM_Led_UNO_A7670E` | Validación de comunicación y señalización básica mediante LED     | Prueba intermedia              |
| `HumedadSuelo_TS`               | Lectura de humedad de suelo y envío de datos a ThingSpeak         | Integrado parcialmente         |
| `UNO_A7670E_ThinkSpeak`         | Primera prueba de envío a ThingSpeak mediante A7670E              | Prueba inicial                 |
| `UNO_A7670E_ThinkSpeak_2`       | Segunda iteración de envío HTTP/ThingSpeak                        | Prueba intermedia              |
| `UNO_A7670E_ThinkSpeak_3`       | Tercera iteración de envío HTTP/ThingSpeak                        | Prueba intermedia              |
| `Estacion_Completa_1`           | Integración de sensores, relé, módem LTE y ThingSpeak             | Prototipo integrado            |
| `Estacion_A_4GLTE_OK`           | Versión funcional de la estación A con comunicación 4G/LTE        | Funcional                      |
| `Estacion_A_4GLTE_OK_ahorroRAM` | Versión optimizada para reducir el uso de memoria en Arduino UNO  | Versión recomendada            |


## Documentación del módulo A7670E

La carpeta `doc_a7670e/` contiene documentación propia generada durante las pruebas del módulo LTE.

No incluye drivers, ejecutables ni documentación completa de terceros. En su lugar, recoge las pruebas relevantes y las referencias técnicas utilizadas durante el desarrollo.

### Archivos incluidos

| Archivo                        | Contenido                                                                                                       |
| ------------------------------ | --------------------------------------------------------------------------------------------------------------- |
| `comandos_at_probados.md`      | Secuencia de comandos AT utilizada para validar comunicación, señal, registro LTE, conexión HTTP y envío de SMS |
| `referencias_documentacion.md` | Relación de manuales, especificaciones y documentación técnica consultada sobre el módulo A7670E/A76XX          |

Esta documentación permite justificar técnicamente cómo se validó que el módulo A7670E respondía correctamente, se registraba en red LTE y podía realizar peticiones HTTP reales antes de integrarlo con Arduino.



## Librerías necesarias

Instalar desde el **Library Manager** de Arduino IDE:

* `BH1750`, Christopher Laws.
* `Adafruit BME280 Library`.
* `Adafruit Unified Sensor`.
* `Adafruit BusIO`.

No se versionan las librerías dentro del repositorio para evitar duplicar dependencias externas y mantener el repositorio limpio.


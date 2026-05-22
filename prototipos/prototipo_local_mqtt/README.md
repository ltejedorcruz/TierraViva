# Prototipo local MQTT de TierraViva

Este directorio contiene el prototipo local desarrollado durante una fase inicial del TFG TierraViva.

Su objetivo fue validar la cadena software de recepción, almacenamiento y visualización de datos en Raspberry Pi antes de cerrar la arquitectura remota definitiva basada en LTE, ThingSpeak y estaciones de campo.

## Estado

Este código no corresponde a la arquitectura final del sistema.

Se conserva como evidencia técnica del proceso de desarrollo, ya que permitió validar:

- recepción de telemetría mediante MQTT local;
- almacenamiento en SQLite;
- exposición de datos mediante FastAPI;
- visualización básica en dashboard web;
- integración inicial con datos procedentes de Arduino.

## Diferencia con la arquitectura final

En este prototipo, MQTT se utiliza de forma local dentro de la Raspberry Pi.

En la arquitectura final de TierraViva, las estaciones de campo se comunican remotamente mediante LTE y ThingSpeak. La Raspberry Pi no está conectada físicamente a las estaciones, sino que consulta la nube, registra datos, aplica reglas y genera comandos remotos.

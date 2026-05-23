# Comandos AT probados con el módulo A7670E

Este documento recoge las pruebas realizadas sobre el módulo **SIMCom A7670E** durante el desarrollo de la estación de campo de TierraViva.  
El objetivo de estas pruebas fue validar, antes de la integración completa con Arduino, que el módem respondía correctamente por puerto serie, se registraba en red móvil LTE, obtenía conectividad de datos, permitía realizar peticiones HTTP y era capaz de enviar SMS.



## 1. Condiciones previas de la prueba

Antes de iniciar las pruebas se verificaron las siguientes condiciones:

- Tarjeta SIM insertada, con saldo o datos disponibles.
- PIN de la SIM desactivado.
- Antena LTE conectada al módulo.
- Módulo A7670E conectado al PC mediante USB.
- Terminal serie **SSCOM3.2 / SSCOM32E** abierta sobre el puerto AT del módem.
- Prueba realizada inicialmente sobre el puerto **COM7**.
- Comunicación serie configurada a **115200 baudios**.


## 2. Configuración de SSCOM3.2

En `SSCOM32E` se utilizó la siguiente configuración:

| Parámetro | Valor utilizado |
|---|---|
| Puerto serie | `COM7` |
| Baud rate | `115200` |
| Formato | `8N1` |
| Control de flujo | `None` |
| Final de línea | `Add CRLF` activado |
| Envío hexadecimal | Desactivado |

Después de seleccionar estos parámetros, se abrió el puerto con **Open COM**.

Esta configuración fue necesaria para que el módulo interpretase correctamente los comandos AT, ya que la mayoría de órdenes requieren final de línea con retorno de carro y salto de línea.


## 3. Comprobación básica del módem

### 3.1. Comprobar comunicación AT

**Comando:**

```text
AT
```

**Respuesta esperada:**

```text
OK
```

**Interpretación:**

La respuesta `OK` confirma que existe comunicación serie entre el PC y el módulo, y que el módem está preparado para recibir comandos AT.

---

### 3.2. Identificar el modelo del módulo

**Comando:**

```text
ATI
```

**Respuesta observada:**

```text
A7670E-LASA
```

**Interpretación:**

El comando permitió confirmar que el módulo conectado corresponde a la familia **A7670E**, concretamente identificado como `A7670E-LASA`.


## 4. Pruebas de red LTE

### 4.1. Consultar intensidad de señal

**Comando:**

```text
AT+CSQ
```

**Formato de respuesta:**

```text
+CSQ: <rssi>,<ber>
```

Donde:

- `<rssi>` indica el nivel de señal recibido.
- `<ber>` indica la tasa de error de bit. En LTE suele aparecer como `99`, indicando que no está disponible o no se proporciona.

**Interpretación de RSSI:**

| RSSI | Interpretación |
|---:|---|
| `0-9` | Señal mala |
| `10-14` | Señal baja o aceptable |
| `15-19` | Señal buena |
| `20-31` | Señal muy buena |
| `99` | Señal desconocida o no detectable |

En una de las pruebas se obtuvo un valor de RSSI de `26`, equivalente aproximadamente a **-61 dBm**, lo que indica una señal LTE buena.

---

### 4.2. Comprobar registro en red LTE

**Comando:**

```text
AT+CEREG?
```

**Respuesta observada de referencia:**

```text
+CEREG: 0,5
```

**Interpretación:**

El valor `5` indica que el módulo está **registrado en roaming** dentro de la red LTE.  
Este comportamiento puede ser normal cuando se utilizan operadores móviles virtuales, ya que pueden registrarse sobre la red de un operador anfitrión.

---

### 4.3. Consultar operador y tecnología de acceso

**Comando:**

```text
AT+COPS?
```

**Formato de respuesta:**

```text
+COPS: <mode>,<format>,"<operador>",<act>
```

**Ejemplo de respuesta:**

```text
+COPS: 0,2,"21407",7
```

| Campo | Valor | Significado |
|---|---:|---|
| `<mode>` | `0` | Selección automática de operador |
| `<format>` | `2` | Operador mostrado en formato numérico |
| `<operador>` | `21407` | Código MCC/MNC del operador |
| `<act>` | `7` | Tecnología LTE / E-UTRAN |

El código `21407` se descompone de la siguiente forma:

```text
214 = MCC España
07  = MNC Movistar / Telefónica
```

Por tanto, el módem se encontraba registrado en la red de **Movistar/Telefónica España**, utilizando tecnología **LTE / E-UTRAN**.

---

### 4.4. Consultar dirección IP asignada

**Comando:**

```text
AT+CGPADDR=1
```

**Objetivo:**

Comprobar si el contexto de datos activo tiene una dirección IP asignada.

**Interpretación:**

Si el comando devuelve una dirección IP asociada al contexto PDP `1`, el módulo dispone de conectividad de datos a nivel de red móvil.


## 5. Códigos MCC/MNC de referencia en España

En España el **MCC** es `214`. El **MNC** identifica la red móvil concreta dentro del país.

| MCC/MNC | MCC | MNC | Operador / red |
|---|---:|---:|---|
| `21401` | 214 | 01 | Vodafone España |
| `21403` | 214 | 03 | Orange España |
| `21404` | 214 | 04 | Yoigo / Xfera |
| `21405` | 214 | 05 | Telefónica / Movistar, servicios específicos |
| `21406` | 214 | 06 | Vodafone España |
| `21407` | 214 | 07 | Movistar / Telefónica |
| `21408` | 214 | 08 | Euskaltel |
| `21409` | 214 | 09 | Orange España |
| `21411` | 214 | 11 | Orange España |
| `21415` | 214 | 15 | BT España / servicios móviles |
| `21416` | 214 | 16 | Telecable |
| `21417` | 214 | 17 | R Cable / servicios móviles |
| `21419` | 214 | 19 | Simyo / Orange, según asignación histórica |
| `21420` | 214 | 20 | Fonyou / servicios móviles |
| `21421` | 214 | 21 | Jazztel / Orange, según asignación histórica |
| `21422` | 214 | 22 | Digi Mobil |
| `21423` | 214 | 23 | Barablu / operador virtual, histórico |
| `21425` | 214 | 25 | Lycamobile |
| `21427` | 214 | 27 | Truphone |
| `21428` | 214 | 28 | Telefónica IoT / servicios M2M |
| `21433` | 214 | 33 | Eurona / servicios móviles |
| `21434` | 214 | 34 | Aire Networks |
| `21435` | 214 | 35 | ION Mobile |
| `21436` | 214 | 36 | OpenCable |
| `21451` | 214 | 51 | Adamo Telecom |
| `21455` | 214 | 55 | MásMóvil / servicios asociados |
| `21456` | 214 | 56 | LMS / servicios móviles |
| `21457` | 214 | 57 | MásMóvil / servicios móviles |
| `21458` | 214 | 58 | ION Mobile / Aire Networks |
| `21459` | 214 | 59 | Orange / servicios asociados |

> Esta tabla se utiliza como referencia práctica para interpretar la respuesta del comando `AT+COPS?`.


## 6. Prueba de conectividad HTTP

El siguiente bloque de comandos se utilizó para comprobar que el módulo no solo estaba registrado en red LTE, sino que además podía salir a Internet y recibir datos mediante HTTP.

### 6.1. Inicializar servicio HTTP

**Comando:**

```text
AT+HTTPINIT
```

**Respuesta esperada:**

```text
OK
```

---

### 6.2. Configurar URL de prueba

**Comando:**

```text
AT+HTTPPARA="URL","http://httpbin.org/get"
```

**Respuesta esperada:**

```text
OK
```

---

### 6.3. Ejecutar petición HTTP GET

**Comando:**

```text
AT+HTTPACTION=0
```

**Respuesta esperada:**

```text
+HTTPACTION: 0,200,254
```

**Interpretación:**

| Campo | Valor | Significado |
|---|---:|---|
| Método | `0` | Petición HTTP GET |
| Código HTTP | `200` | Respuesta correcta del servidor |
| Tamaño | `254` | Número de bytes disponibles para lectura |

La obtención de un código `200` confirma que el módulo ha podido realizar una petición HTTP real a través de la red móvil.

---

### 6.4. Leer respuesta HTTP

**Comando:**

```text
AT+HTTPREAD=0,254
```

**Resultado esperado:**

Respuesta en formato JSON similar a la devuelta por `httpbin.org/get`.

**Interpretación:**

La lectura del contenido confirma que la petición no solo fue aceptada, sino que el módulo recibió datos desde Internet.

---

### 6.5. Finalizar servicio HTTP

**Comando:**

```text
AT+HTTPTERM
```

**Respuesta esperada:**

```text
OK
```


## 7. Resumen de validación HTTP

Mediante el bloque HTTP se comprobó que:

- El módulo respondía correctamente a comandos AT.
- El módem estaba registrado en red LTE.
- La intensidad de señal era suficiente.
- El módulo podía realizar una petición HTTP GET.
- La petición devolvía un código HTTP `200`.
- Se podían leer datos reales recibidos desde Internet.

Estas pruebas validaron la viabilidad de utilizar el A7670E como módem 4G/LTE para el envío de telemetría desde la estación de campo de TierraViva.


## 8. Prueba de envío de SMS

Aunque la comunicación principal del sistema se basa en datos móviles e integración con ThingSpeak, también se validó la capacidad del módulo para enviar SMS.

### 8.1. Activar modo texto

**Comando:**

```text
AT+CMGF=1
```

**Respuesta esperada:**

```text
OK
```

---

### 8.2. Preparar envío de SMS

**Comando:**

```text
AT+CMGS="+34XXXXXXXXX"
```

**Respuesta esperada:**

```text
>
```

El carácter `>` indica que el módulo está esperando el contenido del mensaje.

---

### 8.3. Escribir mensaje y finalizar

Después de escribir el texto del mensaje, se envía el carácter **Ctrl+Z**, correspondiente al valor hexadecimal `0x1A`.

En `SSCOM32E` se puede enviar de la siguiente forma:

1. Escribir el mensaje.
2. Pulsar **Send**.
3. Activar **SendHex**.
4. Enviar `1A`.
5. Pulsar **Send**.
6. Desactivar **SendHex**.

---

### 8.4. Respuesta esperada

```text
+CMGS: 23

OK
```

**Interpretación:**

La respuesta `+CMGS` seguida de `OK` indica que el SMS fue aceptado por el módulo para su envío.


## 9. Conclusiones de la prueba

Las pruebas realizadas permiten concluir que el módulo **A7670E** es válido para el prototipo de estación de campo de TierraViva, ya que se verificaron los siguientes puntos:

- Comunicación serie correcta mediante comandos AT.
- Identificación correcta del módulo como `A7670E-LASA`.
- Registro en red LTE.
- Detección de operador y tecnología de acceso.
- Obtención de dirección IP.
- Conectividad HTTP funcional.
- Recepción de datos desde Internet.
- Capacidad de envío de SMS.

Estas pruebas justifican la integración posterior del módulo con Arduino UNO para el envío de telemetría a ThingSpeak y, potencialmente, para la recepción de comandos de control remoto.

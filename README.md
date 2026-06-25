<p align="center">
  <img src="docs/assets/tierraviva_banner.svg" alt="TierraViva banner" width="100%"/>
</p>

<h3 align="center">
  Prototipo IoT distribuido para supervisar el entorno, registrar telemetría y apoyar el riego con criterio de ingeniería
</h3>

<p align="center">
  <img src="https://img.shields.io/badge/TFG-Ingenier%C3%ADa%20de%20Telecomunicaci%C3%B3n-1B5E20?style=for-the-badge" alt="TFG"/>
  <img src="https://img.shields.io/badge/Arquitectura-Distribuida-2E7D32?style=for-the-badge" alt="Arquitectura"/>
  <img src="https://img.shields.io/badge/Estado-Prototipo%20funcional-388E3C?style=for-the-badge" alt="Estado"/>
</p>

<p align="center">
  <img src="https://img.shields.io/badge/Arduino-Estaciones%20IoT-00878F?style=flat-square&logo=arduino&logoColor=white" alt="Arduino"/>
  <img src="https://img.shields.io/badge/A7670E--LASA-LTE-1565C0?style=flat-square" alt="LTE"/>
  <img src="https://img.shields.io/badge/ThingSpeak-Telemetr%C3%ADa-F9A825?style=flat-square" alt="ThingSpeak"/>
  <img src="https://img.shields.io/badge/Raspberry%20Pi-Supervisi%C3%B3n-A22846?style=flat-square&logo=raspberrypi&logoColor=white" alt="Raspberry Pi"/>
  <img src="https://img.shields.io/badge/FastAPI-API-009688?style=flat-square&logo=fastapi&logoColor=white" alt="FastAPI"/>
  <img src="https://img.shields.io/badge/SQLite-Hist%C3%B3rico-003B57?style=flat-square&logo=sqlite&logoColor=white" alt="SQLite"/>
</p>

<p align="center">
  <img src="https://readme-typing-svg.demolab.com?font=Inter&weight=600&size=22&duration=3400&pause=900&center=true&vCenter=true&width=1050&color=2E7D32&lines=Estaciones+IoT+aut%C3%B3nomas+con+decisi%C3%B3n+local+de+riego;ThingSpeak+%E2%86%92+Raspberry+Pi+%E2%86%92+SQLite+%E2%86%92+FastAPI+%E2%86%92+Dashboard;Un+TFG+planteado+como+sistema+completo%2C+no+como+simple+maqueta+con+sensores" alt="Typing SVG" />
</p>


<p align="center">
  <img src="docs/assets/section_one_phrase.svg" alt="TierraViva en una frase" width="100%"/>
</p>

> **TierraViva es un sistema IoT de monitorización ambiental y riego inteligente en el que la actuación crítica se decide localmente en cada estación, mientras que la Raspberry Pi se encarga de supervisar, almacenar, exponer y visualizar la información.**


<p align="center">
  <img src="docs/assets/section_what_is.svg" alt="¿Qué es TierraViva?" width="100%"/>
</p>

**TierraViva** es un prototipo IoT concebido para pequeños cultivos, huertos o espacios agrícolas que no pueden supervisarse de forma presencial continua.

El proyecto aborda una necesidad muy concreta:  
**conocer qué está ocurriendo en el entorno de cultivo, registrar la evolución de sus variables y actuar con seguridad cuando el suelo realmente lo requiere**.

Para ello, integra en un único sistema:

- **estaciones remotas autónomas** basadas en Arduino;
- **sensórica ambiental y de suelo**;
- **comunicación LTE** mediante A7670E-LASA;
- **publicación de telemetría en ThingSpeak**;
- **ingesta y almacenamiento local en Raspberry Pi**;
- **API REST con FastAPI**;
- **dashboard web de supervisión**;
- **documentación técnica, validación y trazabilidad**.

No es solo una maqueta con sensores: es un **prototipo funcional completo**, concebido como un sistema de ingeniería de extremo a extremo.

<p align="center">
  <img src="docs/assets/idea_key.svg" alt="Idea clave de TierraViva" width="100%"/>
</p>

Esto significa que cada estación:

1. **mide** sus variables;
2. **interpreta** las condiciones locales;
3. **decide** si debe actuar;
4. **actúa** mediante relé y bomba de agua;
5. **publica** telemetría, estado y diagnóstico.

La Raspberry Pi **no envía órdenes de riego**.  
Su papel es **supervisar**, **almacenar histórico**, **servir la API** y **hacer visible el comportamiento del sistema**.

Esta separación es una de las decisiones más importantes del proyecto, porque evita que una caída de la supervisión o una incidencia en la conectividad provoquen una actuación física incoherente.


<p align="center">
  <img src="docs/assets/section_highlights.svg" alt="¿Por qué este proyecto destaca?" width="100%"/>
</p>

<table>
  <tr>
    <td width="25%" align="center">
      <h3>🌱 Problema real</h3>
      <p>Parte de una necesidad concreta en pequeños cultivos desatendidos: observar el entorno y apoyar el riego sin depender de presencia física continua.</p>
    </td>
    <td width="25%" align="center">
      <h3>🧠 Criterio de ingeniería</h3>
      <p>La arquitectura evita delegar la decisión crítica en la nube y sitúa la actuación donde está el hardware.</p>
    </td>
    <td width="25%" align="center">
      <h3>🔗 Sistema completo</h3>
      <p>No se limita al nodo sensórico: integra comunicaciones, almacenamiento, API, dashboard, validación y documentación.</p>
    </td>
    <td width="25%" align="center">
      <h3>📘 Documentación técnica</h3>
      <p>Incluye memoria técnica, anexos, trazabilidad, manuales, justificación de decisiones y validación por bloques.</p>
    </td>
  </tr>
</table>


<p align="center">
  <img src="docs/assets/section_system_overview_header.svg" alt="Vista general del sistema" width="100%"/>
</p>

<p align="center">
  <img src="docs/assets/system_overview.svg" alt="Vista general del sistema TierraViva" width="900"/>
</p>

| Capa | Elementos | Función principal |
|---|---|---|
| **Estaciones remotas** | Arduino UNO, BME280, BH1750, sensor de humedad de suelo, A7670E-LASA, relé, bomba 12 V | Medición, decisión local y actuación |
| **Telemetría** | ThingSpeak | Recepción y consulta de datos |
| **Supervisión** | Raspberry Pi + `main.py` + `database.py` | Ingesta, normalización y persistencia |
| **Backend** | `api.py` con FastAPI | API REST y servicio del frontend |
| **Visualización** | `dashboard/` | Consulta de estado, histórico y diagnóstico |

<p align="center">
  <img src="docs/assets/section_architecture_functional.svg" alt="Arquitectura funcional" width="100%"/>
</p>


TierraViva puede entenderse como una cadena funcional muy clara:

```text
[ Estación remota ]
   ├─ Sensores: suelo, temperatura, humedad, luz y presión
   ├─ Evaluación local de condiciones
   ├─ Actuación mediante relé y bomba
   └─ Publicación LTE → ThingSpeak

[ Raspberry Pi ]
   ├─ Consulta periódica a ThingSpeak
   ├─ Normalización de datos
   ├─ Almacenamiento en SQLite
   ├─ API REST con FastAPI
   └─ Servicio del dashboard web

[ Usuario ]
   └─ Consulta estado, histórico, eventos y diagnóstico desde navegador
```


<p align="center">
  <img src="docs/assets/section_thingspeak_contract.svg" alt="Contrato de datos en ThingSpeak" width="100%"/>
</p>

Cada estación publica sus medidas y estado con la siguiente estructura:

| Campo    | Variable interna    | Descripción                    |
| -------- | ------------------- | ------------------------------ |
| `field1` | `soil_moisture_pct` | Humedad de suelo normalizada   |
| `field2` | `temperature_c`     | Temperatura ambiente           |
| `field3` | `humidity_pct`      | Humedad relativa ambiental     |
| `field4` | `light_lux`         | Luminosidad                    |
| `field5` | `pressure_hpa`      | Presión atmosférica            |
| `field6` | `relay_state`       | Estado del relé                |
| `field7` | `system_event`      | Evento operativo               |
| `field8` | `debug_code`        | Código auxiliar de diagnóstico |


<p align="center">
  <img src="docs/assets/section_dashboard.svg" alt="Dashboard" width="100%"/>
</p>

El dashboard de supervisión permite consultar de forma clara:

* última lectura disponible de cada estación;
* humedad del suelo y variables ambientales;
* estado del relé;
* evento operativo y código de diagnóstico;
* evolución reciente de las mediciones;
* histórico almacenado;
* frescura de los datos.

<p align="center">
  <img src="memoria/figs_png/evidencia_dashboard.png" alt="Dashboard TierraViva" width="920"/>
</p>

<p align="center">
  <img src="docs/assets/setup_header.svg" alt="Puesta en marcha rápida" width="100%"/>
</p>
<details>
<summary><strong>Ver pasos de instalación</strong></summary>

### 1. Clonar el repositorio

```bash
git clone https://github.com/ltejedorcruz/TierraViva.git
cd TierraViva
```

### 2. Preparar el fichero de configuración

```bash
cp .env.example .env
nano .env
```

Completa, al menos, los canales y claves de lectura de ThingSpeak:

```env
THINGSPEAK_CHANNEL_A=
THINGSPEAK_READ_KEY_A=
THINGSPEAK_CHANNEL_B=
THINGSPEAK_READ_KEY_B=
```

### 3. Instalar dependencias y preparar el entorno

```bash
chmod +x tierraviva.sh
./tierraviva.sh install
```

Este script:

* crea el entorno virtual;
* instala dependencias Python;
* prepara la base de datos SQLite;
* sincroniza el dashboard en `frontend/`.

### 4. Arrancar el sistema

```bash
./tierraviva.sh start
```

### 5. Acceder al dashboard y la API

Dashboard local:

```text
http://127.0.0.1:8000
```

Ejemplo de consulta a la API:

```bash
curl "http://127.0.0.1:8000/api/latest?station=A"
```

</details>


<p align="center">
  <img src="docs/assets/section_api.svg" alt="API principal" width="100%"/>
</p>


| Endpoint                                | Descripción                                |
| --------------------------------------- | ------------------------------------------ |
| `GET /`                                 | Dashboard web                              |
| `GET /api/config`                       | Configuración visible por el frontend      |
| `GET /api/latest?station=A`             | Última lectura de una estación             |
| `GET /api/readings?station=A&limit=120` | Histórico reciente                         |
| `GET /api/stats?station=A&hours=24`     | Resumen estadístico                        |
| `GET /api/health`                       | Estado básico de la API y la base de datos |


<p align="center">
  <img src="docs/assets/section_repo_structure.svg" alt="Estructura del repositorio" width="100%"/>
</p>

```text
.
├── api.py                          # API FastAPI y servicio del dashboard
├── database.py                     # Persistencia y consultas SQLite
├── main.py                         # Ingesta periódica desde ThingSpeak
├── tierraviva.sh                   # Instalación, arranque y gestión del sistema
├── .env.example                    # Plantilla de configuración
├── requirements.txt                # Dependencias Python del proyecto
├── README.md                       # Presentación general del repositorio
│
├── dashboard/                      # Dashboard web de supervisión
│   ├── images/                     # Logo TierraViva, logo URJC y favicon
│   └── index.html                  # Interfaz principal del dashboard
│
├── docs/                           # Documentación auxiliar y recursos del README
│   ├── assets/                     # Assets gráficos del README
│   ├── Circuit_Documentation_TierraViva.pdf
│   └── logs_arduino_estacionA.txt
│
├── Estacion/                       # Firmware final de las estaciones remotas
│   ├── Estacion_A_final_Plus/      # Código Arduino de la estación A
│   └── Estacion_B_final_Plus/      # Código Arduino de la estación B
│
├── memoria/                        # Memoria del TFG en LaTeX
│   ├── anexos/                     # Anexos técnicos, manuales y trazabilidad
│   ├── capitulos/                  # Capítulos principales de la memoria
│   ├── figs/                       # Figuras originales y recursos gráficos
│   ├── figs_png/                   # Figuras exportadas y capturas utilizadas
│   ├── portada/                    # Portada, resumen, prólogo e índices
│   ├── bibliografia.bib            # Bibliografía del trabajo
│   ├── estilo.tex                  # Configuración de estilo LaTeX
│   ├── memoria.tex                 # Documento principal de la memoria
│   └── memoria.pdf                 # Versión compilada de la memoria
│
└── prototipos/                     # Prototipos previos y pruebas de evolución
    ├── estacion_pruebas/           # Ensayos iniciales con A7670E y sensores
    └── prototipo_local_mqtt/       # Arquitectura previa basada en MQTT local
```

<p align="center">
  <img src="docs/assets/docs_header.svg" alt="Documentación técnica" width="100%"/>
</p>

La documentación completa del proyecto se encuentra en la carpeta `memoria/`.

La memoria incluye:

* introducción y estado del arte;
* requisitos, materiales y decisiones;
* arquitectura final;
* implementación hardware y firmware;
* implementación software;
* validación, resultados y discusión;
* manual de instalación;
* manual de usuario;
* manual API;
* glosario;
* trazabilidad requisitos-pruebas.


<p align="center">
  <img src="docs/assets/footer.svg" alt="TierraViva footer" width="100%"/>
</p>

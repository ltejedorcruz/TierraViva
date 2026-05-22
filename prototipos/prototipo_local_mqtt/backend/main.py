import json
import paho.mqtt.client as mqtt
import database

# Configuración (provisional: usamos hum_pct como “humedad” para el dashboard)
UMBRAL_HUMEDAD_AIRE = 35.0  # %HR (esto NO es humedad del suelo; lo ajustaremos luego)

TOPIC_TELEMETRY = "tierraviva/stations/+/telemetry"
TOPIC_ACTUADORES_BASE = "tierraviva/actuadores"  # dejamos ya TierraViva

def on_message(client, userdata, msg):
    try:
        data = json.loads(msg.payload.decode("utf-8", errors="ignore"))

        station = data.get("station", "desconocida")
        temp = float(data.get("temp_c", 0))
        hum = float(data.get("hum_pct", 0))

        print(f"-> Estación {station}: T={temp}ºC, HR={hum}% (topic={msg.topic})")

        # 1) Guardar en DB
        soil_raw = data.get("soil_raw", None)
        lux = data.get("lux", None)
        pres_hpa = data.get("pres_hpa", None)

        # Normalizamos tipos por si vienen como strings
        try:
            soil_raw = int(soil_raw) if soil_raw is not None else None
        except Exception:
            soil_raw = None

        try:
            lux = float(lux) if lux is not None else None
        except Exception:
            lux = None

        try:
            pres_hpa = float(pres_hpa) if pres_hpa is not None else None
        except Exception:
            pres_hpa = None

        database.guardar_lectura(
            station, temp, hum,
            soil_raw=soil_raw, lux=lux, pres_hpa=pres_hpa
        )

        # 2) Lógica (provisional): basada en humedad del aire
        if hum < UMBRAL_HUMEDAD_AIRE:
            print(f"   [!] HR baja en {station}. (Provisional) Publicando ON...")
            client.publish(f"{TOPIC_ACTUADORES_BASE}/{station}", "ON")
        else:
            client.publish(f"{TOPIC_ACTUADORES_BASE}/{station}", "OFF")

    except Exception as e:
        print(f"Error procesando mensaje MQTT: {e} | payload={msg.payload!r}")

# Inicio
database.init_db()
client = mqtt.Client(callback_api_version=mqtt.CallbackAPIVersion.VERSION2)
client.on_message = on_message
client.connect("localhost", 1883)
client.subscribe(TOPIC_TELEMETRY)

print("TierraViva Backend: Operativo y escuchando en", TOPIC_TELEMETRY)
client.loop_forever()

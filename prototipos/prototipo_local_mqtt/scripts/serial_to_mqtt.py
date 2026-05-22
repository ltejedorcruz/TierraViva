import json
import time
import serial
import paho.mqtt.client as mqtt

SERIAL_PORT = "/dev/ttyUSB0"
BAUDRATE = 115200

MQTT_HOST = "127.0.0.1"
MQTT_PORT = 1883

BASE_TOPIC = "tierraviva/stations"  # topic del sistema

def main():
    client = mqtt.Client()
    client.connect(MQTT_HOST, MQTT_PORT, keepalive=60)
    client.loop_start()

    while True:
        try:
            with serial.Serial(SERIAL_PORT, BAUDRATE, timeout=2) as ser:
                # Vacía líneas basura al arrancar el Arduino/serial
                time.sleep(2)
                ser.reset_input_buffer()

                while True:
                    line = ser.readline().decode("utf-8", errors="ignore").strip()
                    if not line:
                        continue

                    # Comentarios/boot del Arduino
                    if line.startswith("#"):
                        print(f"BOOT: {line}")
                        continue

                    # Solo aceptamos JSON en una línea
                    if not (line.startswith("{") and line.endswith("}")):
                        print(f"SKIP: {line}")
                        continue

                    try:
                        payload = json.loads(line)
                    except json.JSONDecodeError:
                        print(f"BADJSON: {line}")
                        continue

                    station = payload.get("station", "unknown")
                    topic = f"{BASE_TOPIC}/{station}/telemetry"

                    client.publish(topic, json.dumps(payload), qos=0, retain=False)
                    print(f"MQTT -> {topic}: {payload}")

        except serial.SerialException as e:
            print(f"Serial error: {e}. Retrying in 3s...")
            time.sleep(3)

if __name__ == "__main__":
    main()

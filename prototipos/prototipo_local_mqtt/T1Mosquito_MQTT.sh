# Arrancamos mosquito
sudo systemctl start mosquitto
sudo systemctl enable mosquitto
sudo systemctl status mosquitto --no-pager

# Bridge Serie → MQTT (Arduino → MQTT)
source ~/smartland/.venv/bin/activate
python3 ~/smartland/scripts/serial_to_mqtt.py


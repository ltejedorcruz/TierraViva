import sqlite3
import os

DB_PATH = os.path.expanduser("~/smartland/data/smartland.db")

def init_db():
    conn = sqlite3.connect(DB_PATH)
    cursor = conn.cursor()
    # Tabla de lecturas de sensores
    cursor.execute('''CREATE TABLE IF NOT EXISTS lecturas 
	                (id INTEGER PRIMARY KEY AUTOINCREMENT, 
	                timestamp DATETIME DEFAULT CURRENT_TIMESTAMP,
	                sensor_id TEXT, 
	                temperatura REAL, 
	                humedad REAL,
	                soil_raw INTEGER,
	                lux REAL,
	                pres_hpa REAL)''')

    # Tabla de registro de riego (para saber cuándo se regó)
    cursor.execute('''CREATE TABLE IF NOT EXISTS riego_log 
                      (id INTEGER PRIMARY KEY AUTOINCREMENT, 
                       timestamp DATETIME DEFAULT CURRENT_TIMESTAMP,
                       zona TEXT, 
                       duracion_seg INTEGER)''')
    conn.commit()
    conn.close()

def guardar_lectura(sensor_id, temp, hum, soil_raw=None, lux=None, pres_hpa=None):
    conn = sqlite3.connect(DB_PATH)
    cursor = conn.cursor()
    cursor.execute(
        "INSERT INTO lecturas (sensor_id, temperatura, humedad, soil_raw, lux, pres_hpa) VALUES (?, ?, ?, ?, ?, ?)",
        (sensor_id, temp, hum, soil_raw, lux, pres_hpa)
    )
    conn.commit()
    conn.close()

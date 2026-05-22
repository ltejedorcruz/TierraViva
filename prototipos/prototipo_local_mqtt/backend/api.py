from fastapi import FastAPI
from fastapi.responses import FileResponse
import sqlite3
import os

app = FastAPI()
DB_PATH = os.path.expanduser("~/smartland/data/smartland.db")

def get_db_connection():
    conn = sqlite3.connect(DB_PATH)
    conn.row_factory = sqlite3.Row
    return conn

# 1. La Web (Dashboard)
@app.get("/")
async def read_index():
    return FileResponse(os.path.expanduser("~/smartland/frontend/index.html"))

# 2. Los Datos (JSON)
@app.get("/lecturas")
def obtener_lecturas(limit: int = 10):
    conn = get_db_connection()
    cursor = conn.cursor()
    cursor.execute("SELECT * FROM lecturas ORDER BY timestamp DESC LIMIT ?", (limit,))
    lecturas = [dict(row) for row in cursor.fetchall()]
    conn.close()
    return lecturas

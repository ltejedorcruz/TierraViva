from __future__ import annotations

import os
from datetime import datetime, timezone
from pathlib import Path
from typing import Optional

from fastapi import FastAPI, HTTPException, Query
from fastapi.responses import FileResponse
from fastapi.staticfiles import StaticFiles

from database import (
    BASE_DIR,
    FRONTEND_DIR,
    DB_PATH,
    get_latest_reading,
    get_readings,
    get_stats,
    init_db,
    get_state,
)


DEFAULT_STATION = os.getenv("STATION_ID", "A").strip().upper() or "A"
STATIONS_ENV = os.getenv("STATIONS", "A,B").strip()
ACTIVE_STATIONS = [s.strip().upper() for s in STATIONS_ENV.split(",") if s.strip()]

SOIL_LOW_THRESHOLD = float(os.getenv("SOIL_LOW_THRESHOLD", "35"))
SOIL_HIGH_THRESHOLD = float(os.getenv("SOIL_HIGH_THRESHOLD", "45"))
WEB_REFRESH_SECONDS = int(os.getenv("WEB_REFRESH_SECONDS", "3"))
DATA_STALE_SECONDS = int(os.getenv("DATA_STALE_SECONDS", "90"))

app = FastAPI(title="TierraViva Dashboard", version="2.0.0")

app.mount("/css", StaticFiles(directory=str(FRONTEND_DIR / "css")), name="css")
app.mount("/js", StaticFiles(directory=str(FRONTEND_DIR / "js")), name="js")
app.mount("/images", StaticFiles(directory=str(FRONTEND_DIR / "images")), name="images")


def normalize_station(station: Optional[str]) -> str:
    if station is None or not str(station).strip():
        return DEFAULT_STATION
    return str(station).strip().upper()


def parse_iso(ts: str):
    try:
        return datetime.fromisoformat(ts.replace("Z", "+00:00"))
    except Exception:
        return None


def age_seconds_from_ts(ts: str) -> int | None:
    dt = parse_iso(ts)
    if dt is None:
        return None
    return int((datetime.now(timezone.utc) - dt.astimezone(timezone.utc)).total_seconds())


def freshness_label(age_seconds: int | None) -> str:
    if age_seconds is None:
        return "desconocido"
    if age_seconds <= DATA_STALE_SECONDS / 2:
        return "fresh"
    if age_seconds <= DATA_STALE_SECONDS:
        return "stale"
    return "offline"


def soil_label(soil: float | None) -> str:
    if soil is None:
        return "sin_dato"
    if soil < SOIL_LOW_THRESHOLD:
        return "seco"
    if soil > SOIL_HIGH_THRESHOLD:
        return "húmedo"
    return "normal"


@app.on_event("startup")
def startup_event() -> None:
    init_db()


@app.get("/")
def index():
    index_path = FRONTEND_DIR / "index.html"
    if not index_path.exists():
        raise HTTPException(status_code=404, detail="No se encontró frontend/index.html")
    return FileResponse(str(index_path))

@app.get("/favicon.ico")
def favicon():
    favicon_path = FRONTEND_DIR / "images" / "favicon.ico"
    if not favicon_path.exists():
        raise HTTPException(status_code=404, detail="No se encontró favicon.ico")
    return FileResponse(str(favicon_path))
    
    
@app.get("/api/config")
def api_config(station: Optional[str] = Query(None)):
    selected_station = normalize_station(station)
    return {
        "ok": True,
        "station_id": selected_station,
        "active_stations": ACTIVE_STATIONS,
        "soil_low_threshold": SOIL_LOW_THRESHOLD,
        "soil_high_threshold": SOIL_HIGH_THRESHOLD,
        "web_refresh_seconds": WEB_REFRESH_SECONDS,
        "data_stale_seconds": DATA_STALE_SECONDS,
        "base_dir": str(BASE_DIR),
        "database_path": str(DB_PATH),
        "last_ingest_ts": get_state(f"last_ingest_ts_{selected_station}"),
        "last_entry_id": get_state(f"last_entry_id_{selected_station}"),
    }


@app.get("/api/latest")
def api_latest(station: Optional[str] = Query(None)):
    selected_station = normalize_station(station)
    reading = get_latest_reading(selected_station)
    if reading is None:
        return {
            "ok": False,
            "station_id": selected_station,
            "reading": None,
            "status": "sin_datos",
        }

    age_seconds = age_seconds_from_ts(reading["ts"])
    soil = reading.get("soil_moisture_pct")

    return {
        "ok": True,
        "station_id": selected_station,
        "reading": reading,
        "age_seconds": age_seconds,
        "freshness": freshness_label(age_seconds),
        "soil_state": soil_label(soil),
    }


@app.get("/api/readings")
def api_readings(
    station: Optional[str] = Query(None),
    limit: int = Query(120, ge=1, le=500),
):
    selected_station = normalize_station(station)
    readings = get_readings(limit=limit, station_id=selected_station)
    return {
        "ok": True,
        "station_id": selected_station,
        "count": len(readings),
        "readings": readings,
    }


@app.get("/api/stats")
def api_stats(
    station: Optional[str] = Query(None),
    hours: int = Query(24, ge=1, le=720),
):
    selected_station = normalize_station(station)
    stats = get_stats(hours=hours, station_id=selected_station)
    return {
        "ok": True,
        **stats,
    }


@app.get("/api/health")
def api_health(station: Optional[str] = Query(None)):
    selected_station = normalize_station(station)
    latest = get_latest_reading(selected_station)
    return {
        "ok": True,
        "station_id": selected_station,
        "active_stations": ACTIVE_STATIONS,
        "database": str(DB_PATH),
        "frontend_exists": (FRONTEND_DIR / "index.html").exists(),
        "has_data": latest is not None,
    }
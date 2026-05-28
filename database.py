from __future__ import annotations

import json
import os
import sqlite3
from datetime import datetime, timedelta, timezone
from pathlib import Path
from typing import Any, Dict, List, Optional


BASE_DIR = Path(os.getenv("TIERRAVIVA_HOME", str(Path.home() / "tierraviva"))).expanduser()
DATA_DIR = BASE_DIR / "data"
FRONTEND_DIR = BASE_DIR / "frontend"
DB_PATH = DATA_DIR / "tierraviva.db"


def utc_now_iso() -> str:
    return datetime.now(timezone.utc).replace(microsecond=0).isoformat().replace("+00:00", "Z")


def ensure_dirs() -> None:
    DATA_DIR.mkdir(parents=True, exist_ok=True)
    FRONTEND_DIR.mkdir(parents=True, exist_ok=True)


def get_connection() -> sqlite3.Connection:
    ensure_dirs()
    conn = sqlite3.connect(DB_PATH, timeout=30)
    conn.row_factory = sqlite3.Row
    conn.execute("PRAGMA journal_mode=WAL;")
    conn.execute("PRAGMA foreign_keys=ON;")
    return conn


def _table_columns(conn: sqlite3.Connection, table_name: str) -> set[str]:
    rows = conn.execute(f"PRAGMA table_info({table_name})").fetchall()
    return {row["name"] for row in rows}


def _ensure_column(conn: sqlite3.Connection, table_name: str, column_sql: str, column_name: str) -> None:
    existing = _table_columns(conn, table_name)
    if column_name not in existing:
        conn.execute(f"ALTER TABLE {table_name} ADD COLUMN {column_sql}")


def init_db() -> None:
    with get_connection() as conn:
        conn.executescript(
            """
            CREATE TABLE IF NOT EXISTS readings (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                entry_id INTEGER,
                station_id TEXT NOT NULL,
                ts TEXT NOT NULL,
                soil_moisture_pct REAL,
                temperature_c REAL,
                humidity_pct REAL,
                light_lux REAL,
                pressure_hpa REAL,
                relay_state INTEGER,
                system_event INTEGER,
                debug_code INTEGER,
                source TEXT NOT NULL DEFAULT 'thingspeak',
                raw_json TEXT,
                UNIQUE(station_id, entry_id)
            );

            CREATE INDEX IF NOT EXISTS idx_readings_station_ts
            ON readings(station_id, ts DESC);

            CREATE TABLE IF NOT EXISTS valve_events (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                ts TEXT NOT NULL,
                station_id TEXT NOT NULL,
                event_type TEXT NOT NULL,
                source TEXT,
                duration_seconds REAL,
                notes TEXT
            );

            CREATE INDEX IF NOT EXISTS idx_valve_events_station_ts
            ON valve_events(station_id, ts DESC);

            CREATE TABLE IF NOT EXISTS system_state (
                key TEXT PRIMARY KEY,
                value TEXT NOT NULL,
                updated_at TEXT NOT NULL
            );
            """
        )

        # Soft migration for older databases
        _ensure_column(conn, "readings", "relay_state INTEGER", "relay_state")
        _ensure_column(conn, "readings", "system_event INTEGER", "system_event")
        _ensure_column(conn, "readings", "debug_code INTEGER", "debug_code")

        conn.commit()


def set_state(key: str, value: str) -> None:
    now = utc_now_iso()
    with get_connection() as conn:
        conn.execute(
            """
            INSERT INTO system_state (key, value, updated_at)
            VALUES (?, ?, ?)
            ON CONFLICT(key) DO UPDATE SET
                value=excluded.value,
                updated_at=excluded.updated_at
            """,
            (key, value, now),
        )
        conn.commit()


def get_state(key: str, default: Optional[str] = None) -> Optional[str]:
    with get_connection() as conn:
        row = conn.execute("SELECT value FROM system_state WHERE key = ?", (key,)).fetchone()
        if row is None:
            return default
        return row["value"]


def insert_reading(
    station_id: str,
    entry_id: Optional[int],
    soil_moisture_pct: Optional[float],
    temperature_c: Optional[float],
    humidity_pct: Optional[float],
    light_lux: Optional[float],
    pressure_hpa: Optional[float] = None,
    relay_state: Optional[int] = None,
    system_event: Optional[int] = None,
    debug_code: Optional[int] = None,
    source: str = "thingspeak",
    raw_json: Optional[dict] = None,
    ts: Optional[str] = None,
) -> bool:
    """Devuelve True si insertó algo nuevo, False si era duplicado."""
    ensure_dirs()
    ts_value = ts or utc_now_iso()
    raw_text = json.dumps(raw_json, ensure_ascii=False) if raw_json is not None else None

    with get_connection() as conn:
        cur = conn.execute(
            """
            INSERT OR IGNORE INTO readings (
                entry_id, station_id, ts,
                soil_moisture_pct, temperature_c, humidity_pct,
                light_lux, pressure_hpa,
                relay_state, system_event, debug_code,
                source, raw_json
            )
            VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
            """,
            (
                entry_id,
                station_id,
                ts_value,
                soil_moisture_pct,
                temperature_c,
                humidity_pct,
                light_lux,
                pressure_hpa,
                relay_state,
                system_event,
                debug_code,
                source,
                raw_text,
            ),
        )
        conn.commit()
        return cur.rowcount > 0


def _row_to_dict(row: sqlite3.Row | None) -> Optional[Dict[str, Any]]:
    if row is None:
        return None
    return dict(row)


def get_latest_reading(station_id: str = "A") -> Optional[Dict[str, Any]]:
    with get_connection() as conn:
        row = conn.execute(
            """
            SELECT *
            FROM readings
            WHERE station_id = ?
            ORDER BY ts DESC, id DESC
            LIMIT 1
            """,
            (station_id,),
        ).fetchone()
        return _row_to_dict(row)


def get_readings(limit: int = 100, station_id: str = "A") -> List[Dict[str, Any]]:
    limit = max(1, min(int(limit), 500))
    with get_connection() as conn:
        rows = conn.execute(
            """
            SELECT *
            FROM readings
            WHERE station_id = ?
            ORDER BY ts DESC, id DESC
            LIMIT ?
            """,
            (station_id, limit),
        ).fetchall()
        return [dict(row) for row in rows]


def get_stats(hours: int = 24, station_id: str = "A") -> Dict[str, Any]:
    hours = max(1, min(int(hours), 720))
    cutoff = (datetime.now(timezone.utc) - timedelta(hours=hours)).replace(microsecond=0).isoformat().replace("+00:00", "Z")

    with get_connection() as conn:
        row = conn.execute(
            """
            SELECT
                COUNT(*) AS samples,

                AVG(soil_moisture_pct) AS soil_avg,
                MIN(soil_moisture_pct) AS soil_min,
                MAX(soil_moisture_pct) AS soil_max,

                AVG(temperature_c) AS temp_avg,
                MIN(temperature_c) AS temp_min,
                MAX(temperature_c) AS temp_max,

                AVG(humidity_pct) AS hum_avg,
                MIN(humidity_pct) AS hum_min,
                MAX(humidity_pct) AS hum_max,

                AVG(light_lux) AS light_avg,
                MIN(light_lux) AS light_min,
                MAX(light_lux) AS light_max,

                AVG(pressure_hpa) AS pres_avg,
                MIN(pressure_hpa) AS pres_min,
                MAX(pressure_hpa) AS pres_max,

                MIN(ts) AS first_ts,
                MAX(ts) AS last_ts
            FROM readings
            WHERE station_id = ? AND ts >= ?
            """,
            (station_id, cutoff),
        ).fetchone()

    if row is None:
        return {}

    return {
        "station_id": station_id,
        "hours": hours,
        "samples": row["samples"],
        "window_start": cutoff,
        "first_ts": row["first_ts"],
        "last_ts": row["last_ts"],
        "soil": {
            "avg": row["soil_avg"],
            "min": row["soil_min"],
            "max": row["soil_max"],
        },
        "temperature": {
            "avg": row["temp_avg"],
            "min": row["temp_min"],
            "max": row["temp_max"],
        },
        "humidity": {
            "avg": row["hum_avg"],
            "min": row["hum_min"],
            "max": row["hum_max"],
        },
        "light": {
            "avg": row["light_avg"],
            "min": row["light_min"],
            "max": row["light_max"],
        },
        "pressure": {
            "avg": row["pres_avg"],
            "min": row["pres_min"],
            "max": row["pres_max"],
        },
    }


def insert_valve_event(
    station_id: str,
    event_type: str,
    source: str = "raspi",
    duration_seconds: Optional[float] = None,
    notes: Optional[str] = None,
    ts: Optional[str] = None,
) -> None:
    ts_value = ts or utc_now_iso()
    with get_connection() as conn:
        conn.execute(
            """
            INSERT INTO valve_events (ts, station_id, event_type, source, duration_seconds, notes)
            VALUES (?, ?, ?, ?, ?, ?)
            """,
            (ts_value, station_id, event_type, source, duration_seconds, notes),
        )
        conn.commit()


def get_valve_events(limit: int = 50, station_id: str = "A") -> List[Dict[str, Any]]:
    limit = max(1, min(int(limit), 200))
    with get_connection() as conn:
        rows = conn.execute(
            """
            SELECT *
            FROM valve_events
            WHERE station_id = ?
            ORDER BY ts DESC, id DESC
            LIMIT ?
            """,
            (station_id, limit),
        ).fetchall()
        return [dict(row) for row in rows]
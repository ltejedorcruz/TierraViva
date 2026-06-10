from __future__ import annotations

import logging
import os
import time
from dataclasses import dataclass
from typing import Any, Dict, List, Optional

import requests

from database import init_db, insert_reading, get_state, set_state, utc_now_iso


LOG_LEVEL = os.getenv("TIERRAVIVA_LOG_LEVEL", "INFO").upper()
logging.basicConfig(level=LOG_LEVEL, format="%(asctime)s [%(levelname)s] %(message)s")
logger = logging.getLogger("tierraviva-ingest")

POLL_INTERVAL_SECONDS = int(os.getenv("POLL_INTERVAL_SECONDS", "30"))
REQUEST_TIMEOUT_SECONDS = int(os.getenv("REQUEST_TIMEOUT_SECONDS", "12"))

STATIONS_ENV = os.getenv("STATIONS", "A,B").strip()
STATION_IDS = [s.strip().upper() for s in STATIONS_ENV.split(",") if s.strip()]

THINGSPEAK_LAST_URL = "https://api.thingspeak.com/channels/{channel_id}/feeds/last.json"


@dataclass(frozen=True)
class StationConfig:
    station_id: str
    channel_id: str
    read_api_key: str


def safe_float(value: Any) -> Optional[float]:
    try:
        if value is None:
            return None
        if isinstance(value, str) and value.strip() == "":
            return None
        return float(value)
    except Exception:
        return None


def safe_int(value: Any) -> Optional[int]:
    try:
        if value is None:
            return None
        if isinstance(value, str) and value.strip() == "":
            return None
        return int(float(value))
    except Exception:
        return None


def safe_text(value: Any) -> Optional[str]:
    if value is None:
        return None
    if isinstance(value, str):
        text = value.strip()
        if text == "" or text.lower() == "null":
            return None
        return text
    return str(value)


def load_station_configs() -> List[StationConfig]:
    configs: List[StationConfig] = []

    for station_id in STATION_IDS:
        channel_id = os.getenv(f"THINGSPEAK_CHANNEL_{station_id}", "").strip()
        read_api_key = os.getenv(f"THINGSPEAK_READ_KEY_{station_id}", "").strip()

        if not channel_id:
            logger.warning("Falta THINGSPEAK_CHANNEL_%s, estación ignorada.", station_id)
            continue

        configs.append(
            StationConfig(
                station_id=station_id,
                channel_id=channel_id,
                read_api_key=read_api_key,
            )
        )

    return configs


def fetch_latest_feed(cfg: StationConfig) -> Dict[str, Any]:
    url = THINGSPEAK_LAST_URL.format(channel_id=cfg.channel_id)
    params: Dict[str, str] = {}
    if cfg.read_api_key:
        params["api_key"] = cfg.read_api_key

    response = requests.get(
        url,
        params=params if params else None,
        timeout=REQUEST_TIMEOUT_SECONDS,
    )
    response.raise_for_status()

    data = response.json()
    if not isinstance(data, dict):
        raise ValueError(f"Respuesta ThingSpeak inválida para estación {cfg.station_id}: {type(data).__name__}")

    return data


def normalize_feed(feed: Dict[str, Any]) -> Dict[str, Any]:
    return {
        "entry_id": safe_int(feed.get("entry_id")),
        "created_at": feed.get("created_at"),
        "soil_moisture_pct": safe_float(feed.get("field1")),
        "temperature_c": safe_float(feed.get("field2")),
        "humidity_pct": safe_float(feed.get("field3")),
        "light_lux": safe_float(feed.get("field4")),
        "pressure_hpa": safe_float(feed.get("field5")),
        "relay_state": safe_int(feed.get("field6")),
        "system_event": safe_int(feed.get("field7")),
        "debug_code": safe_int(feed.get("field8")),
    }


def ingest_station(cfg: StationConfig) -> bool:
    try:
        feed = fetch_latest_feed(cfg)
    except Exception as exc:
        logger.error("Error leyendo ThingSpeak para estación %s: %s", cfg.station_id, exc)
        return False

    normalized = normalize_feed(feed)
    entry_id = normalized["entry_id"]

    if entry_id is None:
        logger.warning("Estación %s: no se recibió entry_id válido.", cfg.station_id)
        return False

    last_entry_id = get_state(f"last_entry_id_{cfg.station_id}")
    if last_entry_id == str(entry_id):
        logger.info("Estación %s: sin cambios (entry_id=%s).", cfg.station_id, entry_id)
        return False

    inserted = insert_reading(
        station_id=cfg.station_id,
        entry_id=entry_id,
        soil_moisture_pct=normalized["soil_moisture_pct"],
        temperature_c=normalized["temperature_c"],
        humidity_pct=normalized["humidity_pct"],
        light_lux=normalized["light_lux"],
        pressure_hpa=normalized["pressure_hpa"],
        relay_state=normalized["relay_state"],
        system_event=normalized["system_event"],
        debug_code=normalized["debug_code"],
        source="thingspeak",
        raw_json=feed,
        ts=utc_now_iso(),
    )

    if inserted:
        now = utc_now_iso()
        set_state(f"last_entry_id_{cfg.station_id}", str(entry_id))
        set_state(f"last_ingest_ts_{cfg.station_id}", now)
        set_state(f"last_channel_id_{cfg.station_id}", cfg.channel_id)

        logger.info(
            "Nueva lectura [%s] entry_id=%s soil=%s temp=%s hum=%s light=%s pres=%s relay=%s event=%s debug=%s",
            cfg.station_id,
            entry_id,
            f"{normalized['soil_moisture_pct']:.1f}" if normalized["soil_moisture_pct"] is not None else "-",
            f"{normalized['temperature_c']:.2f}" if normalized["temperature_c"] is not None else "-",
            f"{normalized['humidity_pct']:.1f}" if normalized["humidity_pct"] is not None else "-",
            f"{normalized['light_lux']:.1f}" if normalized["light_lux"] is not None else "-",
            f"{normalized['pressure_hpa']:.1f}" if normalized["pressure_hpa"] is not None else "-",
            normalized["relay_state"] if normalized["relay_state"] is not None else "-",
            normalized["system_event"] or "-",
            normalized["debug_code"] or "-",
        )
    else:
        logger.info("Estación %s: lectura duplicada ignorada (entry_id=%s).", cfg.station_id, entry_id)

    return inserted


def main() -> None:
    init_db()

    configs = load_station_configs()
    if not configs:
        raise SystemExit("No hay estaciones configuradas. Revisa THINGSPEAK_CHANNEL_A/B y THINGSPEAK_READ_KEY_A/B.")

    logger.info(
        "TierraViva monitor iniciado. Estaciones activas: %s | Intervalo: %ss",
        ", ".join(cfg.station_id for cfg in configs),
        POLL_INTERVAL_SECONDS,
    )

    while True:
        try:
            for cfg in configs:
                ingest_station(cfg)
        except KeyboardInterrupt:
            logger.info("Cerrando ingestor.")
            break
        except Exception as exc:
            logger.exception("Error en el loop principal: %s", exc)

        time.sleep(POLL_INTERVAL_SECONDS)


if __name__ == "__main__":
    main()
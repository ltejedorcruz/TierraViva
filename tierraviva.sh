#!/usr/bin/env bash
# ============================================================
#  TierraViva – Script de despliegue y gestión (Fase 2)
# ============================================================
#  Uso:
#    ./tierraviva.sh install    → Instalar dependencias y crear BD
#    ./tierraviva.sh start      → Arrancar ingestor + API (foreground)
#    ./tierraviva.sh status     → Ver estado de los servicios
#    ./tierraviva.sh stop       → Parar servicios systemd
#    ./tierraviva.sh logs       → Ver logs de los servicios
# ============================================================
set -Eeuo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
VENV_DIR="$ROOT_DIR/venv"
PID_DIR="$ROOT_DIR/pids"
LOG_DIR="$ROOT_DIR/logs"
FRONTEND_DIR="$ROOT_DIR/frontend"

MAIN_PID_FILE="$PID_DIR/main.pid"
API_PID_FILE="$PID_DIR/api.pid"

MAIN_LOG="$LOG_DIR/main.log"
API_LOG="$LOG_DIR/api.log"

PYTHON_BIN="${PYTHON_BIN:-python3}"
PY_BIN="$VENV_DIR/bin/python"
UVICORN_BIN="$VENV_DIR/bin/uvicorn"

MAIN_PID=""
API_PID=""
TAIL_PID=""
MONITOR_PID=""
CLEANED_UP=0

mkdir -p "$PID_DIR" "$LOG_DIR" "$FRONTEND_DIR"

print_header() {
  echo "=================================================="
  echo " TierraViva"
  echo "=================================================="
}

msg() {
  echo "[TierraViva] $*"
}

load_env_file() {
  if [ -f "$ROOT_DIR/.env" ]; then
    set -a
    # shellcheck disable=SC1090
    source "$ROOT_DIR/.env"
    set +a
  fi
}

ensure_venv() {
  if [ ! -d "$VENV_DIR" ]; then
    msg "Creando entorno virtual..."
    "$PYTHON_BIN" -m venv "$VENV_DIR"
  fi
}

activate_venv() {
  # shellcheck disable=SC1091
  source "$VENV_DIR/bin/activate"
}

sync_frontend() {
  mkdir -p "$FRONTEND_DIR"

  if [ -f "$ROOT_DIR/index.html" ]; then
    cp "$ROOT_DIR/index.html" "$FRONTEND_DIR/index.html"
    msg "index.html copiado/actualizado en frontend/index.html"
  else
    msg "AVISO: no existe $ROOT_DIR/index.html, no se pudo copiar al frontend."
  fi
}

install_deps() {
  print_header
  msg "Instalando dependencias..."

  load_env_file
  ensure_venv
  activate_venv

  python -m pip install --upgrade pip >/dev/null

  if [ -f "$ROOT_DIR/requirements.txt" ]; then
    pip install -r "$ROOT_DIR/requirements.txt"
  else
    msg "No existe requirements.txt, instalando paquetes base..."
    pip install fastapi uvicorn requests python-dotenv
  fi

  msg "Inicializando base de datos..."
  python -c "from database import init_db; init_db()"

  msg "Preparando frontend..."
  sync_frontend

  msg "Instalación completada."
}

require_env_vars() {
  local missing=()

  for var in THINGSPEAK_CHANNEL_A THINGSPEAK_CHANNEL_B THINGSPEAK_READ_KEY_A THINGSPEAK_READ_KEY_B; do
    if [ -z "${!var:-}" ]; then
      missing+=("$var")
    fi
  done

  if [ "${#missing[@]}" -gt 0 ]; then
    echo
    echo "[TierraViva] ERROR: faltan variables obligatorias en .env:"
    for v in "${missing[@]}"; do
      echo "  - $v"
    done
    echo
    echo "[TierraViva] Revisa que el fichero $ROOT_DIR/.env exista y que esos nombres estén escritos exactamente igual."
    return 1
  fi
}

ensure_runtime_ready() {
  load_env_file
  ensure_venv
  activate_venv

  if [ ! -x "$PY_BIN" ] || [ ! -x "$UVICORN_BIN" ]; then
    msg "Entorno incompleto. Ejecutando instalación automática..."
    install_deps
    load_env_file
    activate_venv
    return
  fi

  if ! python - <<'PY' >/dev/null 2>&1; then
import fastapi
import uvicorn
import requests
import dotenv
PY
    msg "Dependencias incompletas. Reinstalando..."
    install_deps
    load_env_file
    activate_venv
  fi
}

cleanup_pidfile_if_stale() {
  local pid_file="$1"
  if [ -f "$pid_file" ]; then
    local pid
    pid="$(cat "$pid_file" 2>/dev/null || true)"
    if [ -z "${pid:-}" ] || ! kill -0 "$pid" 2>/dev/null; then
      rm -f "$pid_file"
    fi
  fi
}

is_running() {
  local pid_file="$1"
  if [ -f "$pid_file" ]; then
    local pid
    pid="$(cat "$pid_file" 2>/dev/null || true)"
    if [ -n "${pid:-}" ] && kill -0 "$pid" 2>/dev/null; then
      return 0
    fi
  fi
  return 1
}

stop_process_from_pidfile() {
  local pid_file="$1"
  local name="$2"

  if [ -f "$pid_file" ]; then
    local pid
    pid="$(cat "$pid_file" 2>/dev/null || true)"

    if [ -n "${pid:-}" ] && kill -0 "$pid" 2>/dev/null; then
      msg "Deteniendo $name (PID $pid)..."
      kill "$pid" 2>/dev/null || true

      for _ in $(seq 1 20); do
        if ! kill -0 "$pid" 2>/dev/null; then
          break
        fi
        sleep 0.2
      done

      if kill -0 "$pid" 2>/dev/null; then
        msg "$name no respondió, forzando cierre..."
        kill -9 "$pid" 2>/dev/null || true
      fi
    fi

    rm -f "$pid_file"
    msg "$name detenido."
  else
    msg "No hay PID de $name."
  fi
}

cleanup() {
  if [ "$CLEANED_UP" -eq 1 ]; then
    return
  fi
  CLEANED_UP=1

  trap - INT TERM

  if [ -n "${MONITOR_PID:-}" ] && kill -0 "$MONITOR_PID" 2>/dev/null; then
    kill "$MONITOR_PID" 2>/dev/null || true
  fi

  if [ -n "${TAIL_PID:-}" ] && kill -0 "$TAIL_PID" 2>/dev/null; then
    kill "$TAIL_PID" 2>/dev/null || true
  fi

  stop_process_from_pidfile "$API_PID_FILE" "api.py"
  stop_process_from_pidfile "$MAIN_PID_FILE" "main.py"
}

start_main() {
  cleanup_pidfile_if_stale "$MAIN_PID_FILE"

  if is_running "$MAIN_PID_FILE"; then
    MAIN_PID="$(cat "$MAIN_PID_FILE")"
    msg "main.py ya está ejecutándose."
    return 0
  fi

  msg "Arrancando main.py..."
  (
    cd "$ROOT_DIR"
    "$PY_BIN" "$ROOT_DIR/main.py" >> "$MAIN_LOG" 2>&1 &
    echo $! > "$MAIN_PID_FILE"
  )

  MAIN_PID="$(cat "$MAIN_PID_FILE")"
  msg "main.py arrancado (PID $MAIN_PID)."
}

start_api() {
  cleanup_pidfile_if_stale "$API_PID_FILE"

  if is_running "$API_PID_FILE"; then
    API_PID="$(cat "$API_PID_FILE")"
    msg "api.py ya está ejecutándose."
    return 0
  fi

  msg "Arrancando api.py..."
  (
    cd "$ROOT_DIR"
    "$UVICORN_BIN" api:app --host 0.0.0.0 --port 8000 >> "$API_LOG" 2>&1 &
    echo $! > "$API_PID_FILE"
  )

  API_PID="$(cat "$API_PID_FILE")"
  msg "api.py arrancado (PID $API_PID)."
}

monitor_services() {
  while true; do
    if [ -n "${MAIN_PID:-}" ] && ! kill -0 "$MAIN_PID" 2>/dev/null; then
      echo
      msg "main.py ha terminado."
      kill "$TAIL_PID" 2>/dev/null || true
      return 1
    fi

    if [ -n "${API_PID:-}" ] && ! kill -0 "$API_PID" 2>/dev/null; then
      echo
      msg "api.py ha terminado."
      kill "$TAIL_PID" 2>/dev/null || true
      return 1
    fi

    sleep 1
  done
}

start_services() {
  print_header
  ensure_runtime_ready
  sync_frontend

  load_env_file
  if ! require_env_vars; then
    return 1
  fi

  : > "$MAIN_LOG"
  : > "$API_LOG"

  start_main
  start_api

  msg "Servicios iniciados correctamente."
  msg "API: http://127.0.0.1:8000"
  msg "Pulsa Ctrl+C para parar todo."
  echo

  trap cleanup INT TERM

  tail -n 0 -F "$MAIN_LOG" "$API_LOG" &
  TAIL_PID=$!

  monitor_services &
  MONITOR_PID=$!

  wait "$TAIL_PID" || true

  cleanup
}

stop_services() {
  print_header
  cleanup
  msg "Servicios detenidos."
}

status_services() {
  print_header

  if is_running "$MAIN_PID_FILE"; then
    echo "[OK] main.py en ejecución (PID $(cat "$MAIN_PID_FILE"))"
  else
    echo "[--] main.py no está ejecutándose"
  fi

  if is_running "$API_PID_FILE"; then
    echo "[OK] api.py en ejecución (PID $(cat "$API_PID_FILE"))"
  else
    echo "[--] api.py no está ejecutándose"
  fi

  if [ -f "$FRONTEND_DIR/index.html" ]; then
    echo "[OK] frontend/index.html existe"
  else
    echo "[!!] frontend/index.html no existe"
  fi

  echo
  echo "Comprobación rápida de puerto 8000:"
  if command -v ss >/dev/null 2>&1; then
    if ss -ltn 2>/dev/null | grep -q ':8000 '; then
      echo "[OK] Puerto 8000 escuchando"
    else
      echo "[--] Puerto 8000 no aparece en escucha"
    fi
  else
    echo "[--] comando ss no disponible"
  fi
}

show_logs() {
  print_header
  msg "Mostrando logs. Ctrl+C para salir."
  echo "--------------------------------------------------"
  tail -n 100 -f "$MAIN_LOG" "$API_LOG"
}

restart_services() {
  stop_services
  sleep 1
  start_services
}

case "${1:-}" in
  install)
    install_deps
    ;;
  start)
    start_services
    ;;
  stop)
    stop_services
    ;;
  restart)
    restart_services
    ;;
  status)
    status_services
    ;;
  logs)
    show_logs
    ;;
  *)
    echo "Uso: $0 {install|start|stop|restart|status|logs}"
    exit 1
    ;;
esac

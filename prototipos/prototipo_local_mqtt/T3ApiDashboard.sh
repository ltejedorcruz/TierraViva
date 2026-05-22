# Arrancamos en otra Terminal el API DashBoard para mostrarlo por el puerto 8000 en navegador
source ~/smartland/.venv/bin/activate
cd ~/smartland/backend
uvicorn api:app --host 0.0.0.0 --port 8000


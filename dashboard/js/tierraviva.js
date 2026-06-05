let config = {
      station_id: "A",
      active_stations: ["A", "B"],
      soil_low_threshold: 35,
      soil_high_threshold: 45,
      web_refresh_seconds: 3,
      data_stale_seconds: 90
    };

    let refreshTimer = null;
    let currentStation = "A";

    function isFiniteNumber(value) {
      return value !== null && value !== undefined && value !== "" && Number.isFinite(Number(value));
    }

    function formatNumber(value, digits = 1, suffix = "") {
      if (!isFiniteNumber(value)) return "—";
      return `${Number(value).toFixed(digits)}${suffix}`;
    }

    function formatDate(ts) {
      if (!ts) return "—";
      const d = new Date(ts);
      if (isNaN(d.getTime())) return String(ts);
      return d.toLocaleString("es-ES");
    }

    function setConnection(state, text) {
      const dot = document.getElementById("connectionDot");
      const pillText = document.getElementById("connectionText");
      dot.className = "dot " + (state || "");
      pillText.textContent = text;
    }

    function soilLabel(soil) {
      if (!isFiniteNumber(soil)) return { label: "Sin dato", cls: "badge bad" };
      const value = Number(soil);
      if (value < Number(config.soil_low_threshold)) return { label: "Seco", cls: "badge warn" };
      if (value > Number(config.soil_high_threshold)) return { label: "Húmedo", cls: "badge ok" };
      return { label: "Normal", cls: "badge ok" };
    }

    function parseRawJson(rawJson) {
      if (!rawJson) return {};
      if (typeof rawJson === "object") return rawJson;
      if (typeof rawJson !== "string") return {};
      try {
        return JSON.parse(rawJson);
      } catch {
        return {};
      }
    }

    function getTelemetryMeta(reading) {
      const raw = parseRawJson(reading?.raw_json);
      return {
        relay_state: reading?.relay_state ?? raw?.field6 ?? raw?.relay_state ?? null,
        system_event: reading?.system_event ?? raw?.field7 ?? raw?.system_event ?? null,
        debug_code: reading?.debug_code ?? raw?.field8 ?? raw?.debug_code ?? null,
      };
    }

    function relayBadge(value) {
      const normalized = String(value ?? "").trim();
      if (normalized === "1") return { label: "Activo", cls: "badge ok" };
      if (normalized === "0") return { label: "OFF", cls: "badge warn" };
      if (!normalized) return { label: "Sin dato", cls: "badge bad" };
      return { label: normalized, cls: "badge ok" };
    }

    function normalizeFieldValue(value) {
      const normalized = String(value ?? "").trim();
      return normalized && normalized !== "null" ? normalized : "";
    }


    const DEBUG_CODE_LABELS = {
      "200": "OK",
      "204": "no riego",
      "206": "modo seguro",
      "400": "error",
      "429": "Cooldown Activo"
    };

    const EVENT_CODE_LABELS = {
      "100": "Reposo / sin evento",
      "206": "Modo seguro activo",
      "210": "Riego ON",
      "211": "Riego OFF",
      "400": "error",
      "429": "Cooldown Activo"
    };

    function translateCode(value, labels) {
      const normalized = normalizeFieldValue(value);
      if (!normalized) return "—";
      return labels[normalized] ? `${normalized} · ${labels[normalized]}` : normalized;
    }

    function translateDebugCode(value) {
      return translateCode(value, DEBUG_CODE_LABELS);
    }

    function translateEventCode(value) {
      return translateCode(value, EVENT_CODE_LABELS);
    }

    function mostFrequentValue(values) {
      const counts = new Map();
      let bestValue = "";
      let bestCount = 0;

      values.forEach((value) => {
        const current = counts.get(value) || 0;
        const next = current + 1;
        counts.set(value, next);
        if (next > bestCount) {
          bestCount = next;
          bestValue = value;
        }
      });

      return { value: bestValue, count: bestCount, total: values.length };
    }

    function renderStats(stats, readings = []) {
      const grid = document.getElementById("statsGrid");
      if (!stats || !stats.samples) {
        grid.innerHTML = `<div class="card mini-stat"><h3>Sin histórico todavía</h3><div class="rows"><span><em>Esperando datos...</em></span></div></div>`;
        return;
      }

      const statCard = (title, obj, suffix = "") => `
        <div class="card mini-stat">
          <h3>${title}</h3>
          <div class="rows">
            <span><strong>Media</strong><span>${formatNumber(obj.avg, 1, suffix)}</span></span>
            <span><strong>Mín</strong><span>${formatNumber(obj.min, 1, suffix)}</span></span>
            <span><strong>Máx</strong><span>${formatNumber(obj.max, 1, suffix)}</span></span>
          </div>
        </div>
      `;

      const relayValues = readings.map(r => normalizeFieldValue(getTelemetryMeta(r).relay_state)).filter(Boolean);
      const eventValues = readings.map(r => normalizeFieldValue(getTelemetryMeta(r).system_event)).filter(Boolean);
      const debugValues = readings.map(r => normalizeFieldValue(getTelemetryMeta(r).debug_code)).filter(Boolean);

      const relayOnCount = relayValues.filter(v => v === "1").length;
      const relayOffCount = relayValues.filter(v => v === "0").length;
      const relayLatest = relayValues.length ? relayValues[relayValues.length - 1] : "";
      const eventDominant = mostFrequentValue(eventValues);
      const debugDominant = mostFrequentValue(debugValues);
      const eventLatest = eventValues.length ? eventValues[eventValues.length - 1] : "";
      const debugLatest = debugValues.length ? debugValues[debugValues.length - 1] : "";

      grid.innerHTML = `
        <div class="card mini-stat">
          <h3>Muestras</h3>
          <div class="rows">
            <span><strong>Total</strong><span>${stats.samples}</span></span>
            <span><strong>Desde</strong><span>${formatDate(stats.first_ts)}</span></span>
            <span><strong>Hasta</strong><span>${formatDate(stats.last_ts)}</span></span>
          </div>
        </div>
        ${statCard("Humedad del suelo", stats.soil, "%")}
        ${statCard("Temperatura", stats.temperature, " °C")}
        ${statCard("Humedad ambiental", stats.humidity, "%")}
        ${statCard("Luminosidad", stats.light, " lx")}
        ${statCard("Presión", stats.pressure, " hPa")}
        <div class="card mini-stat">
          <h3>Relé activo</h3>
          <div class="rows">
            <span><strong>ON</strong><span>${relayOnCount}</span></span>
            <span><strong>OFF</strong><span>${relayOffCount}</span></span>
            <span><strong>Último</strong><span>${relayLatest || "—"}</span></span>
          </div>
        </div>
        <div class="card mini-stat">
          <h3>Estado / evento</h3>
          <div class="rows">
            <span><strong>Dominante</strong><span>${translateEventCode(eventDominant.value)}</span></span>
            <span><strong>Repeticiones</strong><span>${eventDominant.count || 0}/${eventDominant.total || 0}</span></span>
            <span><strong>Último</strong><span>${translateEventCode(eventLatest)}</span></span>
          </div>
        </div>
        <div class="card mini-stat">
          <h3>Debug / código</h3>
          <div class="rows">
            <span><strong>Dominante</strong><span>${translateDebugCode(debugDominant.value)}</span></span>
            <span><strong>Repeticiones</strong><span>${debugDominant.count || 0}/${debugDominant.total || 0}</span></span>
            <span><strong>Último</strong><span>${translateDebugCode(debugLatest)}</span></span>
          </div>
        </div>
      `;
    }

    function renderReadingsTable(readings) {
      const tbody = document.getElementById("readingsBody");
      if (!readings || !readings.length) {
        tbody.innerHTML = `<tr><td colspan="6" class="muted">Sin lecturas todavía.</td></tr>`;
        return;
      }

      tbody.innerHTML = readings.map(r => `
        <tr>
          <td>${formatDate(r.ts)}</td>
          <td>${formatNumber(r.soil_moisture_pct, 1, "%")}</td>
          <td>${formatNumber(r.temperature_c, 2, " °C")}</td>
          <td>${formatNumber(r.humidity_pct, 1, "%")}</td>
          <td>${formatNumber(r.light_lux, 1, " lx")}</td>
          <td>${formatNumber(r.pressure_hpa, 1, " hPa")}</td>
        </tr>
      `).join("");
    }

    function renderChart(svgId, readings, key, color, suffix = "", digits = 1) {
      const svg = document.getElementById(svgId);
      const values = (readings || [])
        .slice()
        .reverse()
        .map(r => Number(r[key]))
        .filter(v => Number.isFinite(v));

      if (!values.length) {
        svg.innerHTML = `<text x="300" y="110" text-anchor="middle" fill="#6b7c74" font-size="18">Sin datos</text>`;
        return;
      }

      const width = 600;
      const height = 220;
      const padL = 40;
      const padR = 15;
      const padT = 20;
      const padB = 35;

      const min = Math.min(...values);
      const max = Math.max(...values);
      const range = (max - min) || 1;
      const n = values.length;
      const stepX = (width - padL - padR) / Math.max(n - 1, 1);

      const points = values.map((v, i) => {
        const x = padL + i * stepX;
        const y = height - padB - ((v - min) / range) * (height - padT - padB);
        return { x, y, v };
      });

      const line = points.map((p, i) => `${i === 0 ? "M" : "L"} ${p.x.toFixed(1)} ${p.y.toFixed(1)}`).join(" ");
      const area = `${line} L ${points[points.length - 1].x.toFixed(1)} ${height - padB} L ${points[0].x.toFixed(1)} ${height - padB} Z`;

      const ticks = 4;
      const yMarks = [];
      for (let i = 0; i <= ticks; i++) {
        const v = min + (range * i / ticks);
        const y = height - padB - (height - padT - padB) * i / ticks;
        yMarks.push(`<line x1="${padL}" y1="${y}" x2="${width - padR}" y2="${y}" stroke="#edf2ef" stroke-width="1"/>`);
        yMarks.push(`<text x="10" y="${y + 4}" fill="#6b7c74" font-size="11">${v.toFixed(digits)}${suffix}</text>`);
      }

      const xLast = points[points.length - 1];
      const latest = values[values.length - 1];

      svg.innerHTML = `
        <defs>
          <linearGradient id="${svgId}-grad" x1="0" y1="0" x2="0" y2="1">
            <stop offset="0%" stop-color="${color}" stop-opacity="0.35"/>
            <stop offset="100%" stop-color="${color}" stop-opacity="0.03"/>
          </linearGradient>
        </defs>
        ${yMarks.join("")}
        <path d="${area}" fill="url(#${svgId}-grad)"></path>
        <path d="${line}" fill="none" stroke="${color}" stroke-width="3" stroke-linecap="round" stroke-linejoin="round"></path>
        <circle cx="${xLast.x.toFixed(1)}" cy="${xLast.y.toFixed(1)}" r="4.5" fill="${color}"></circle>
        <text x="${width - 20}" y="18" text-anchor="end" fill="${color}" font-size="16" font-weight="700">${latest.toFixed(digits)}${suffix}</text>
        <text x="${padL}" y="${height - 10}" fill="#6b7c74" font-size="11">${values.length} puntos</text>
      `;
    }

    async function fetchJson(url) {
      const response = await fetch(url, { cache: "no-store" });
      if (!response.ok) throw new Error(`HTTP ${response.status} en ${url}`);
      return await response.json();
    }

    function setStationOptions(stations, selected) {
      const select = document.getElementById("stationSelect");
      const active = (stations && stations.length) ? stations : ["A", "B"];
      select.innerHTML = active.map(s => `<option value="${s}">${s}</option>`).join("");
      select.value = selected || active[0] || "A";
    }

    async function refreshDashboard() {
      try {
        const [cfg, latestResp, readingsResp, statsResp] = await Promise.all([
          fetchJson(`/api/config?station=${encodeURIComponent(currentStation)}`),
          fetchJson(`/api/latest?station=${encodeURIComponent(currentStation)}`),
          fetchJson(`/api/readings?station=${encodeURIComponent(currentStation)}&limit=10`),
          fetchJson(`/api/stats?station=${encodeURIComponent(currentStation)}&hours=24`),
        ]);

        config = cfg;
        currentStation = cfg.station_id || currentStation;

        setStationOptions(cfg.active_stations || ["A", "B"], currentStation);

        document.getElementById("soilThresholds").textContent =
          `${Number(config.soil_low_threshold).toFixed(0)}% / ${Number(config.soil_high_threshold).toFixed(0)}%`;

        document.getElementById("footerNote").textContent =
          `Auto-refresco cada ${config.web_refresh_seconds ?? 3} segundos · Estación ${config.station_id}`;

        renderStats(statsResp, readingsResp.readings || []);
        renderReadingsTable(readingsResp.readings || []);

        const latest = latestResp.reading;
        if (!latestResp.ok || !latest) {
          setConnection("offline", "Sin datos");
          document.getElementById("soilValue").textContent = "—";
          document.getElementById("tempValue").textContent = "—";
          document.getElementById("humValue").textContent = "—";
          document.getElementById("lightValue").textContent = "—";
          document.getElementById("pressureValue").textContent = "—";
          document.getElementById("relayValue").textContent = "—";
          document.getElementById("eventValue").textContent = "—";
          document.getElementById("debugValue").textContent = "—";
          document.getElementById("lastUpdateValue").textContent = "—";
          document.getElementById("lastSeenValue").textContent = "Esperando la primera lectura...";
          document.getElementById("soilStateLabel").className = "badge bad";
          document.getElementById("soilStateLabel").textContent = "Sin dato";
          ["chartSoil","chartTemp","chartHum","chartLight","chartPressure"].forEach(id => {
            document.getElementById(id).innerHTML = `<text x="300" y="110" text-anchor="middle" fill="#6b7c74" font-size="18">Sin datos</text>`;
          });
          return;
        }

        const age = latestResp.age_seconds;
        const freshness = latestResp.freshness || "desconocido";
        const soil = latest.soil_moisture_pct;
        const meta = getTelemetryMeta(latest);

        document.getElementById("soilValue").textContent = formatNumber(soil, 1, "%");
        document.getElementById("tempValue").textContent = formatNumber(latest.temperature_c, 2, "°");
        document.getElementById("humValue").textContent = formatNumber(latest.humidity_pct, 1, "%");
        document.getElementById("lightValue").textContent = formatNumber(latest.light_lux, 1, "");
        document.getElementById("pressureValue").textContent = formatNumber(latest.pressure_hpa, 1, "");

        document.getElementById("relayValue").textContent = String(meta.relay_state ?? "—");
        document.getElementById("eventValue").textContent = translateEventCode(meta.system_event);
        document.getElementById("debugValue").textContent = translateDebugCode(meta.debug_code);

        const soilBadge = soilLabel(soil);
        const soilLabelEl = document.getElementById("soilStateLabel");
        soilLabelEl.className = soilBadge.cls;
        soilLabelEl.textContent = soilBadge.label;

        document.getElementById("lastUpdateValue").textContent = freshness.toUpperCase();
        document.getElementById("lastSeenValue").textContent =
          `Última lectura: ${formatDate(latest.ts)} · hace ${age ?? "?"} s · entry_id=${latest.entry_id ?? "?"}`;

        if (freshness === "fresh") {
          setConnection("online", "En vivo");
        } else if (freshness === "stale") {
          setConnection("stale", "Datos recientes");
        } else {
          setConnection("offline", "Sin actualización");
        }

        const readings = readingsResp.readings || [];
        renderChart("chartSoil", readings, "soil_moisture_pct", "#7e6733", "%", 1);
        renderChart("chartTemp", readings, "temperature_c", "#b76a3d", "°C", 1);
        renderChart("chartHum", readings, "humidity_pct", "#4d8795", "%", 1);
        renderChart("chartLight", readings, "light_lux", "#ad913a", " lx", 0);
        renderChart("chartPressure", readings, "pressure_hpa", "#6c6f8f", " hPa", 1);
        renderChart("chartRelay", readings, "relay_state", "#407b69", "", 0);
        renderChart("chartEvent", readings, "system_event", "#285d37", "", 0);
        renderChart("chartDebug", readings, "debug_code", "#2b7864", "", 0);
      } catch (error) {
        console.error(error);
        setConnection("offline", "Error de conexión");
      }
    }

    document.getElementById("stationSelect").addEventListener("change", (ev) => {
      currentStation = ev.target.value;
      refreshDashboard();
    });

    fetchJson("/api/config")
      .then(cfg => {
        config = cfg;
        currentStation = cfg.station_id || "A";
        setStationOptions(cfg.active_stations || ["A", "B"], currentStation);
        refreshDashboard();
        if (refreshTimer) clearInterval(refreshTimer);
        refreshTimer = setInterval(refreshDashboard, (cfg.web_refresh_seconds || 3) * 1000);
      })
      .catch(() => {
        setStationOptions(["A", "B"], currentStation);
        refreshDashboard();
        refreshTimer = setInterval(refreshDashboard, 3000);
      });
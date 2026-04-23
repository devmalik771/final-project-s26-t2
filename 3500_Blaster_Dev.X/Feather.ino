/*
 * FriendlyFire — ESP32-S2 WebSocket Bridge
 * 
 * Serves the referee console web page and runs a WebSocket server.
 * For now: sends fake game state updates every 2 seconds.
 * Later: reads UART from vest ATmega328PB and relays real data.
 * 
 * Board: Adafruit Feather ESP32-S2
 * Libraries: WebSockets by Markus Sattler
 */

#include <WiFi.h>
#include <WebServer.h>
#include <WebSocketsServer.h>

// ===== Wi-Fi credentials =====
  // const char* ssid     = "esesyno";
  // const char* password = "eniacpenn";

// ===== Servers =====
WebServer server(80);           // HTTP server for the web page
WebSocketsServer webSocket(81); // WebSocket server on port 81

// ===== UART bridge =====
const int blasterRxPin = 16;
const int blasterTxPin = -1;
String blasterLine;

// ===== Game state =====
int p1Health = 100;
int p1Ammo   = 12;
int p2Health = 100;
int p2Ammo   = 12;
bool matchRunning = false;
unsigned long lastUpdate = 0;

void broadcastJSON(const char* event, int target);
void handleBlasterLine(const String& line);
void pollBlasterUART();

// ===== Web page (embedded) =====
const char webpage[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>FriendlyFire — Referee Console</title>
<style>
  @import url('https://fonts.googleapis.com/css2?family=Share+Tech+Mono&family=Rajdhani:wght@400;600;700&display=swap');

  :root {
    --bg: #0a0c10;
    --surface: #12151c;
    --surface2: #1a1e28;
    --border: #2a2f3a;
    --text: #e0e0e0;
    --text-dim: #6b7280;
    --accent: #f59e0b;
    --red: #ef4444;
    --red-glow: rgba(239, 68, 68, 0.3);
    --green: #22c55e;
    --green-glow: rgba(34, 197, 94, 0.3);
    --blue: #3b82f6;
  }

  * { margin: 0; padding: 0; box-sizing: border-box; }

  body {
    background: var(--bg);
    color: var(--text);
    font-family: 'Rajdhani', sans-serif;
    min-height: 100vh;
  }

  body::after {
    content: '';
    position: fixed;
    top: 0; left: 0; right: 0; bottom: 0;
    background: repeating-linear-gradient(
      transparent, transparent 2px,
      rgba(0,0,0,0.08) 2px, rgba(0,0,0,0.08) 4px
    );
    pointer-events: none;
    z-index: 999;
  }

  .console { max-width: 800px; margin: 0 auto; padding: 20px; }

  .header {
    display: flex; justify-content: space-between; align-items: center;
    padding: 16px 20px; background: var(--surface);
    border: 1px solid var(--border); border-radius: 8px; margin-bottom: 20px;
  }

  .header h1 {
    font-family: 'Share Tech Mono', monospace;
    font-size: 14px; letter-spacing: 3px; text-transform: uppercase;
    color: var(--accent);
  }

  .connection-status {
    display: flex; align-items: center; gap: 8px;
    font-family: 'Share Tech Mono', monospace; font-size: 12px; letter-spacing: 1px;
  }

  .status-dot {
    width: 8px; height: 8px; border-radius: 50%;
    background: var(--red);
    transition: background 0.3s, box-shadow 0.3s;
  }

  .status-dot.connected {
    background: var(--green);
    box-shadow: 0 0 8px var(--green-glow);
    animation: pulse-dot 2s ease-in-out infinite;
  }

  @keyframes pulse-dot {
    0%, 100% { opacity: 1; } 50% { opacity: 0.4; }
  }

  .timer-section { text-align: center; padding: 30px 0; margin-bottom: 20px; }

  .match-timer {
    font-family: 'Share Tech Mono', monospace;
    font-size: 80px; font-weight: 700; color: var(--text);
    text-shadow: 0 0 30px rgba(255,255,255,0.1); line-height: 1;
  }

  .match-status {
    font-family: 'Share Tech Mono', monospace;
    font-size: 13px; letter-spacing: 4px; color: var(--text-dim);
    margin-top: 8px; text-transform: uppercase;
  }

  .match-status.active { color: var(--green); }
  .match-status.ended { color: var(--red); }

  .players { display: grid; grid-template-columns: 1fr 1fr; gap: 16px; margin-bottom: 20px; }

  .player-card {
    background: var(--surface); border: 1px solid var(--border);
    border-radius: 8px; padding: 20px; position: relative;
    overflow: hidden; transition: border-color 0.3s;
  }

  .player-card.hit { border-color: var(--red); box-shadow: 0 0 20px var(--red-glow); }
  .player-card.eliminated { opacity: 0.5; }

  .player-name {
    font-family: 'Share Tech Mono', monospace;
    font-size: 12px; letter-spacing: 2px; color: var(--text-dim);
    text-transform: uppercase; margin-bottom: 16px;
  }

  .player-name span { color: var(--accent); }

  .stats-row { display: grid; grid-template-columns: 1fr 1fr; gap: 12px; margin-bottom: 16px; }

  .stat-box {
    background: var(--surface2); border-radius: 6px; padding: 12px; text-align: center;
  }

  .stat-label {
    font-size: 10px; letter-spacing: 2px; color: var(--text-dim);
    text-transform: uppercase; margin-bottom: 4px;
    font-family: 'Share Tech Mono', monospace;
  }

  .stat-value {
    font-family: 'Share Tech Mono', monospace;
    font-size: 32px; font-weight: 700; line-height: 1;
  }

  .stat-value.health { color: var(--green); }
  .stat-value.health.low { color: var(--accent); }
  .stat-value.health.critical { color: var(--red); }
  .stat-value.ammo { color: var(--blue); }

  .health-bar-container {
    width: 100%; height: 6px; background: var(--surface2);
    border-radius: 3px; overflow: hidden; margin-bottom: 8px;
  }

  .health-bar {
    height: 100%; border-radius: 3px;
    transition: width 0.5s ease, background 0.3s; background: var(--green);
  }

  .health-bar.low { background: var(--accent); }
  .health-bar.critical { background: var(--red); }

  .player-status {
    font-family: 'Share Tech Mono', monospace;
    font-size: 11px; letter-spacing: 1px; text-align: center;
    padding: 4px 8px; border-radius: 4px; background: var(--surface2); color: var(--text-dim);
  }

  .player-status.alive { color: var(--green); }
  .player-status.hit-flash { color: var(--red); background: rgba(239,68,68,0.1); }
  .player-status.dead { color: var(--red); background: rgba(239,68,68,0.15); }

  .event-log {
    background: var(--surface); border: 1px solid var(--border);
    border-radius: 8px; padding: 16px 20px; margin-bottom: 20px;
    max-height: 250px; overflow-y: auto;
  }

  .event-log h2 {
    font-family: 'Share Tech Mono', monospace;
    font-size: 12px; letter-spacing: 2px; color: var(--text-dim); margin-bottom: 12px;
  }

  .event {
    display: flex; align-items: center; gap: 10px; padding: 6px 0;
    border-bottom: 1px solid rgba(255,255,255,0.03);
    font-family: 'Share Tech Mono', monospace; font-size: 13px;
    animation: event-in 0.3s ease-out;
  }

  @keyframes event-in {
    from { opacity: 0; transform: translateY(-8px); }
    to { opacity: 1; transform: translateY(0); }
  }

  .event-time { color: var(--text-dim); font-size: 12px; min-width: 50px; }

  .event-tag {
    font-size: 10px; padding: 2px 6px; border-radius: 3px;
    letter-spacing: 1px; font-weight: 700;
  }

  .event-tag.hit { background: rgba(239,68,68,0.2); color: var(--red); }
  .event-tag.sys { background: rgba(59,130,246,0.2); color: var(--blue); }
  .event-tag.kill { background: rgba(245,158,11,0.2); color: var(--accent); }
  .event-tag.reload { background: rgba(34,197,94,0.2); color: var(--green); }

  .event-text { color: var(--text); }

  .controls { display: grid; grid-template-columns: 1fr 1fr 1fr; gap: 12px; }

  .btn {
    font-family: 'Share Tech Mono', monospace;
    font-size: 13px; letter-spacing: 2px; text-transform: uppercase;
    padding: 14px 20px; border: 1px solid var(--border); border-radius: 6px;
    background: var(--surface); color: var(--text); cursor: pointer;
    transition: all 0.2s;
  }

  .btn:hover { background: var(--surface2); border-color: var(--accent); color: var(--accent); }
  .btn:active { transform: scale(0.97); }
  .btn.start { border-color: var(--green); color: var(--green); }
  .btn.start:hover { background: rgba(34,197,94,0.1); box-shadow: 0 0 15px var(--green-glow); }
  .btn.end { border-color: var(--red); color: var(--red); }
  .btn.end:hover { background: rgba(239,68,68,0.1); }
  .btn.reset { border-color: var(--accent); color: var(--accent); }
  .btn.reset:hover { background: rgba(245,158,11,0.1); }
  .btn:disabled { opacity: 0.3; cursor: not-allowed; }
  .btn:disabled:hover { background: var(--surface); border-color: var(--border); color: var(--text-dim); box-shadow: none; }

  .event-log::-webkit-scrollbar { width: 4px; }
  .event-log::-webkit-scrollbar-track { background: transparent; }
  .event-log::-webkit-scrollbar-thumb { background: var(--border); border-radius: 2px; }

  .mode-badge {
    display: inline-block; margin-top: 20px; padding: 6px 12px;
    border: 1px solid var(--green); border-radius: 4px;
    font-family: 'Share Tech Mono', monospace; font-size: 11px;
    color: var(--green); letter-spacing: 1px; text-align: center; width: 100%;
  }

  @media (max-width: 600px) {
    .players { grid-template-columns: 1fr; }
    .match-timer { font-size: 56px; }
    .controls { grid-template-columns: 1fr; }
  }
</style>
</head>
<body>
<div class="console">
  <div class="header">
    <h1>FriendlyFire Console</h1>
    <div class="connection-status">
      <div class="status-dot" id="statusDot"></div>
      <span id="statusText">Disconnected</span>
    </div>
  </div>

  <div class="timer-section">
    <div class="match-timer" id="timer">00:00</div>
    <div class="match-status" id="matchStatus">Waiting for match</div>
  </div>

  <div class="players">
    <div class="player-card" id="p1Card">
      <div class="player-name">Player 1 — <span>Blaster</span></div>
      <div class="stats-row">
        <div class="stat-box">
          <div class="stat-label">Health</div>
          <div class="stat-value health" id="p1Health">100</div>
        </div>
        <div class="stat-box">
          <div class="stat-label">Ammo</div>
          <div class="stat-value ammo" id="p1Ammo">12</div>
        </div>
      </div>
      <div class="health-bar-container">
        <div class="health-bar" id="p1HealthBar" style="width:100%"></div>
      </div>
      <div class="player-status alive" id="p1Status">ALIVE</div>
    </div>

    <div class="player-card" id="p2Card">
      <div class="player-name">Player 2 — <span>Vest</span></div>
      <div class="stats-row">
        <div class="stat-box">
          <div class="stat-label">Health</div>
          <div class="stat-value health" id="p2Health">100</div>
        </div>
        <div class="stat-box">
          <div class="stat-label">Ammo</div>
          <div class="stat-value ammo" id="p2Ammo">12</div>
        </div>
      </div>
      <div class="health-bar-container">
        <div class="health-bar" id="p2HealthBar" style="width:100%"></div>
      </div>
      <div class="player-status alive" id="p2Status">ALIVE</div>
    </div>
  </div>

  <div class="event-log" id="eventLog">
    <h2>Event Log</h2>
  </div>

  <div class="controls">
    <button class="btn start" id="btnStart" onclick="sendCmd('START')">Start</button>
    <button class="btn end" id="btnEnd" onclick="sendCmd('END')" disabled>End</button>
    <button class="btn reset" id="btnReset" onclick="sendCmd('RESET')">Reset</button>
  </div>

  <div class="mode-badge" id="modeBadge">ESP32 Live Link</div>
</div>

<script>
  // ========== WebSocket ==========
  let ws;
  let matchRunning = false;
  let matchSeconds = 0;
  let timerInterval = null;

  const state = {
    p1: { health: 100, ammo: 12, alive: true },
    p2: { health: 100, ammo: 12, alive: true }
  };

  function connectWebSocket() {
    // Connect to WebSocket on same host, port 81
    const host = window.location.hostname;
    ws = new WebSocket('ws://' + host + ':81');

    ws.onopen = () => {
      document.getElementById('statusDot').className = 'status-dot connected';
      document.getElementById('statusText').textContent = 'Connected';
      addEvent('sys', 'WebSocket connected to ESP32');
    };

    ws.onclose = () => {
      document.getElementById('statusDot').className = 'status-dot';
      document.getElementById('statusText').textContent = 'Disconnected';
      addEvent('sys', 'Connection lost — reconnecting...');
      setTimeout(connectWebSocket, 2000);  // auto-reconnect
    };

    ws.onerror = () => {
      ws.close();
    };

    ws.onmessage = (event) => {
      handleMessage(event.data);
    };
  }

  function handleMessage(raw) {
    // Expected format from ESP32:
    // {"p1h":100,"p1a":12,"p2h":80,"p2a":10,"event":"HIT","target":2}
    try {
      const data = JSON.parse(raw);

      // Update state
      if (data.p1h !== undefined) {
        state.p1.health = data.p1h;
        state.p1.ammo = data.p1a;
        state.p1.alive = data.p1h > 0;
        updatePlayerUI(1);
      }
      if (data.p2h !== undefined) {
        state.p2.health = data.p2h;
        state.p2.ammo = data.p2a;
        state.p2.alive = data.p2h > 0;
        updatePlayerUI(2);
      }

      // Handle events
      if (data.event === 'HIT') {
        flashHit(data.target);
        addEvent('hit', 'Player ' + data.target + ' hit — Health: ' + (data.target === 1 ? data.p1h : data.p2h));
      } else if (data.event === 'KILL') {
        addEvent('kill', 'Player ' + data.target + ' eliminated!');
      } else if (data.event === 'RELOAD') {
        addEvent('reload', 'Player ' + data.target + ' reloaded');
      } else if (data.event === 'START') {
        matchRunning = true;
        matchSeconds = 0;
        document.getElementById('btnStart').disabled = true;
        document.getElementById('btnEnd').disabled = false;
        document.getElementById('matchStatus').textContent = 'Match in progress';
        document.getElementById('matchStatus').className = 'match-status active';
        timerInterval = setInterval(() => {
          matchSeconds++;
          document.getElementById('timer').textContent = formatTime(matchSeconds);
        }, 1000);
        addEvent('sys', 'Match started');
      } else if (data.event === 'END') {
        matchRunning = false;
        clearInterval(timerInterval);
        document.getElementById('btnStart').disabled = true;
        document.getElementById('btnEnd').disabled = true;
        document.getElementById('matchStatus').textContent = 'Match ended';
        document.getElementById('matchStatus').className = 'match-status ended';
        addEvent('sys', 'Match ended');
      } else if (data.event === 'RESET') {
        matchRunning = false;
        clearInterval(timerInterval);
        matchSeconds = 0;
        state.p1 = { health: 100, ammo: 12, alive: true };
        state.p2 = { health: 100, ammo: 12, alive: true };
        updatePlayerUI(1);
        updatePlayerUI(2);
        document.getElementById('timer').textContent = '00:00';
        document.getElementById('matchStatus').textContent = 'Waiting for match';
        document.getElementById('matchStatus').className = 'match-status';
        document.getElementById('btnStart').disabled = false;
        document.getElementById('btnEnd').disabled = true;
        const log = document.getElementById('eventLog');
        log.querySelectorAll('.event').forEach(e => e.remove());
        addEvent('sys', 'System reset — ready');
      }
    } catch (e) {
      console.log('Parse error:', raw);
    }
  }

  function sendCmd(cmd) {
    if (ws && ws.readyState === WebSocket.OPEN) {
      ws.send(cmd);
    }
  }

  // ========== UI ==========
  function updatePlayerUI(num) {
    const p = state['p' + num];
    const healthEl = document.getElementById('p' + num + 'Health');
    const ammoEl = document.getElementById('p' + num + 'Ammo');
    const barEl = document.getElementById('p' + num + 'HealthBar');
    const statusEl = document.getElementById('p' + num + 'Status');
    const cardEl = document.getElementById('p' + num + 'Card');

    healthEl.textContent = p.health;
    ammoEl.textContent = p.ammo;
    barEl.style.width = p.health + '%';

    healthEl.className = 'stat-value health';
    barEl.className = 'health-bar';
    if (p.health <= 20) { healthEl.classList.add('critical'); barEl.classList.add('critical'); }
    else if (p.health <= 50) { healthEl.classList.add('low'); barEl.classList.add('low'); }

    if (!p.alive) {
      statusEl.textContent = 'ELIMINATED';
      statusEl.className = 'player-status dead';
      cardEl.classList.add('eliminated');
    } else {
      statusEl.textContent = 'ALIVE';
      statusEl.className = 'player-status alive';
      cardEl.classList.remove('eliminated');
    }
  }

  function flashHit(num) {
    const card = document.getElementById('p' + num + 'Card');
    card.classList.add('hit');
    setTimeout(() => card.classList.remove('hit'), 500);
    const statusEl = document.getElementById('p' + num + 'Status');
    statusEl.textContent = 'HIT!';
    statusEl.className = 'player-status hit-flash';
    setTimeout(() => updatePlayerUI(num), 800);
  }

  function formatTime(s) {
    const m = Math.floor(s / 60);
    const sec = s % 60;
    return String(m).padStart(2, '0') + ':' + String(sec).padStart(2, '0');
  }

  function addEvent(tag, text) {
    const log = document.getElementById('eventLog');
    const el = document.createElement('div');
    el.className = 'event';
    el.innerHTML =
      '<span class="event-time">' + formatTime(matchSeconds) + '</span>' +
      '<span class="event-tag ' + tag + '">' + tag.toUpperCase() + '</span>' +
      '<span class="event-text">' + text + '</span>';
    const h2 = log.querySelector('h2');
    h2.insertAdjacentElement('afterend', el);
    log.scrollTop = 0;
  }

  // ========== Init ==========
  updatePlayerUI(1);
  updatePlayerUI(2);
  connectWebSocket();
</script>
</body>
</html>
)rawliteral";

// ===== WebSocket event handler =====
void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
  switch (type) {
    case WStype_DISCONNECTED:
      Serial.printf("[WS] Client %u disconnected\n", num);
      break;

    case WStype_CONNECTED:
      Serial.printf("[WS] Client %u connected\n", num);
      break;

    case WStype_TEXT: {
      String msg = String((char*)payload);
      Serial.printf("[WS] Received: %s\n", msg.c_str());

      // Handle commands from web app
      if (msg == "START") {
        matchRunning = true;
        p1Health = 100; p1Ammo = 12;
        p2Health = 100; p2Ammo = 12;
        broadcastJSON("START", 0);
      }
      else if (msg == "END") {
        matchRunning = false;
        broadcastJSON("END", 0);
      }
      else if (msg == "RESET") {
        matchRunning = false;
        p1Health = 100; p1Ammo = 12;
        p2Health = 100; p2Ammo = 12;
        broadcastJSON("RESET", 0);
      }
      break;
    }
  }
}

// ===== Broadcast game state as JSON =====
void broadcastJSON(const char* event, int target) {
  String json = "{";
  json += "\"p1h\":" + String(p1Health) + ",";
  json += "\"p1a\":" + String(p1Ammo) + ",";
  json += "\"p2h\":" + String(p2Health) + ",";
  json += "\"p2a\":" + String(p2Ammo) + ",";
  json += "\"event\":\"" + String(event) + "\",";
  json += "\"target\":" + String(target);
  json += "}";

  webSocket.broadcastTXT(json);
  Serial.println("[TX] " + json);
}

void handleBlasterLine(const String& line) {
  int firstComma = line.indexOf(',');
  int secondComma = line.indexOf(',', firstComma + 1);

  if (firstComma < 0 || secondComma < 0) {
    Serial.println("[UART] Ignoring malformed line: " + line);
    return;
  }

  String player = line.substring(0, firstComma);
  String event = line.substring(firstComma + 1, secondComma);
  int ammo = line.substring(secondComma + 1).toInt();

  if (player != "P1") {
    Serial.println("[UART] Ignoring unsupported player line: " + line);
    return;
  }

  p1Ammo = ammo;
  Serial.println("[UART] " + line);

  if (event == "RELOAD") {
    broadcastJSON("RELOAD", 1);
  } else {
    // Unknown events are still useful because the UI updates ammo
    // from the JSON state before checking event-specific behavior.
    broadcastJSON("AMMO", 1);
  }
}

void pollBlasterUART() {
  while (Serial1.available() > 0) {
    char ch = (char)Serial1.read();

    if (ch == '\r') {
      continue;
    }

    if (ch == '\n') {
      if (blasterLine.length() > 0) {
        handleBlasterLine(blasterLine);
        blasterLine = "";
      }
      continue;
    }

    blasterLine += ch;

    if (blasterLine.length() > 48) {
      Serial.println("[UART] Dropping oversized line");
      blasterLine = "";
    }
  }
}

// ===== Fake game simulation =====
// Remove this later when UART from ATmega is wired up
void fakeGameUpdate() {
  if (!matchRunning) return;
  if (millis() - lastUpdate < 2000) return;  // every 2 seconds
  lastUpdate = millis();

  // Random: someone shoots someone
  int shooter = random(1, 3);  // 1 or 2
  int target = (shooter == 1) ? 2 : 1;

  int* tHealth = (target == 1) ? &p1Health : &p2Health;
  int* sAmmo   = (shooter == 1) ? &p1Ammo : &p2Ammo;

  // Check if shooter needs reload
  if (*sAmmo <= 0) {
    *sAmmo = 12;
    broadcastJSON("RELOAD", shooter);
    return;
  }

  // Shoot
  (*sAmmo)--;

  // Hit (80% chance)
  if (random(100) < 80) {
    *tHealth -= 10;
    if (*tHealth < 0) *tHealth = 0;

    if (*tHealth <= 0) {
      broadcastJSON("KILL", target);
      matchRunning = false;
      // Also send END
      delay(500);
      broadcastJSON("END", 0);
    } else {
      broadcastJSON("HIT", target);
    }
  } else {
    // Miss — just update ammo
    broadcastJSON("HIT", 0);  // no target = miss, just state update
  }
}

// ===== Setup =====
void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n=== FriendlyFire ESP32-S2 ===");

  Serial1.begin(9600, SERIAL_8N2, blasterRxPin, blasterTxPin);
  Serial.println("Blaster UART listening on RX pin " + String(blasterRxPin));

  // Create own access point — no router needed
  WiFi.softAP("FriendlyFire", "lasertag1");
  delay(1000);
  Serial.print("AP started! Connect to Wi-Fi: FriendlyFire");
  Serial.print("  Password: lasertag1");
  Serial.print("  Then open: ");
  Serial.println(WiFi.softAPIP());  // usually 192.168.4.1

  // Serve web page
  server.on("/", []() {
    server.send_P(200, "text/html", webpage);
  });
  server.begin();
  Serial.println("HTTP server started on port 80");

  // Start WebSocket server
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
  Serial.println("WebSocket server started on port 81");
}

// ===== Loop =====
void loop() {
  server.handleClient();
  webSocket.loop();
  pollBlasterUART();
}

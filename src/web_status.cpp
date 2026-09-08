/*
 * Milestone 2 — standalone LAN web status page
 *
 * The ESP32 itself serves a status page on your network. No MQTT, no broker,
 * no Home Assistant, no cloud. Open http://dsc.local/ (or the IP printed on
 * serial) from any browser on the LAN.
 *
 * Deliberately uses the built-in WebServer.h rather than ESPAsyncWebServer:
 * the library's own VirtualKeypad-Web example needs a specific third-party
 * fork of ESPAsyncWebServer plus a SPIFFS data upload step, and that toolchain
 * is fragile on current macOS. Everything here is in-flash and dependency-free.
 *
 * Build:  cp include/secrets.h.example include/secrets.h   (then edit it)
 *         pio run -e web -t upload -t monitor
 *
 * Read-only by default. To enable arming/disarming from the browser you must
 * (a) build the NPN write transistor from docs/wiring.md and
 * (b) uncomment ENABLE_VIRTUAL_KEYPAD below.
 */

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <dscKeybusInterface.h>
#include "secrets.h"

// #define ENABLE_VIRTUAL_KEYPAD

#define dscClockPin 18
#define dscReadPin  19
#define dscWritePin 21

#define ZONE_COUNT 8  // PC1555: 6 on-board zones, up to 8 with keypad zones

// Friendly zone names — edit to match your install
const char* zoneNames[ZONE_COUNT] = {
  "Zone 1", "Zone 2", "Zone 3", "Zone 4",
  "Zone 5", "Zone 6", "Zone 7", "Zone 8"
};

dscKeybusInterface dsc(dscClockPin, dscReadPin, dscWritePin);
WebServer server(80);


// ---------------------------------------------------------------- page ----

static const char INDEX_HTML[] PROGMEM = R"HTML(<!doctype html>
<html lang="en"><head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>DSC Panel</title>
<style>
:root{color-scheme:light dark;--bg:#f6f6f7;--card:#fff;--fg:#16171a;--dim:#6b6f76;
--line:#e3e4e8;--ok:#1a7f4b;--warn:#b8860b;--bad:#c0392b;--okbg:#e7f5ee;--badbg:#fdecea}
@media(prefers-color-scheme:dark){:root{--bg:#0f1012;--card:#17191c;--fg:#e8e9ea;
--dim:#9aa0a6;--line:#2a2d31;--ok:#4ade80;--warn:#fbbf24;--bad:#f87171;
--okbg:#132218;--badbg:#2a1416}}
*{box-sizing:border-box}
body{margin:0;padding:1.25rem;background:var(--bg);color:var(--fg);
font:16px/1.5 -apple-system,BlinkMacSystemFont,"Segoe UI",system-ui,sans-serif}
.wrap{max-width:640px;margin:0 auto}
h1{font-size:1.1rem;margin:0 0 1rem;letter-spacing:.02em;color:var(--dim);font-weight:600}
.card{background:var(--card);border:1px solid var(--line);border-radius:12px;
padding:1rem;margin-bottom:1rem}
.state{font-size:1.6rem;font-weight:650;margin:0}
.sub{color:var(--dim);font-size:.9rem;margin:.25rem 0 0}
.zones{display:grid;grid-template-columns:repeat(auto-fill,minmax(150px,1fr));gap:.5rem}
.z{display:flex;justify-content:space-between;align-items:center;gap:.5rem;
padding:.55rem .7rem;border:1px solid var(--line);border-radius:8px;font-size:.9rem}
.z b{font-weight:500;overflow:hidden;text-overflow:ellipsis;white-space:nowrap}
.tag{font-size:.72rem;text-transform:uppercase;letter-spacing:.06em;
padding:.15rem .45rem;border-radius:5px;white-space:nowrap}
.closed{background:var(--okbg);color:var(--ok)}
.open{background:var(--badbg);color:var(--bad)}
.sys{display:flex;flex-wrap:wrap;gap:.4rem 1.25rem;font-size:.88rem;color:var(--dim)}
.sys span b{font-weight:600}
.good{color:var(--ok)}.bad{color:var(--bad)}.warn{color:var(--warn)}
.pad{display:grid;grid-template-columns:repeat(3,1fr);gap:.5rem;margin-top:.75rem}
.pad button{padding:.85rem;font-size:1.05rem;border:1px solid var(--line);
border-radius:8px;background:var(--bg);color:var(--fg);cursor:pointer}
.pad button:active{background:var(--line)}
#stale{display:none;color:var(--warn);font-size:.85rem;margin-top:.5rem}
</style></head><body><div class="wrap">
<h1>DSC PC1555</h1>

<div class="card">
  <p class="state" id="state">Connecting…</p>
  <p class="sub" id="sub">&nbsp;</p>
  <div id="stale">No response from the panel interface.</div>
</div>

<div class="card"><div class="zones" id="zones"></div></div>

<div class="card"><div class="sys" id="sys"></div></div>

<div class="card" id="keypadCard" hidden>
  <div class="sys"><span>Virtual keypad</span></div>
  <div class="pad">
    <button onclick="k('1')">1</button><button onclick="k('2')">2</button><button onclick="k('3')">3</button>
    <button onclick="k('4')">4</button><button onclick="k('5')">5</button><button onclick="k('6')">6</button>
    <button onclick="k('7')">7</button><button onclick="k('8')">8</button><button onclick="k('9')">9</button>
    <button onclick="k('*')">*</button><button onclick="k('0')">0</button><button onclick="k('#')">#</button>
    <button onclick="k('s')">Stay</button><button onclick="k('w')">Away</button><button onclick="k('c')">Chime</button>
  </div>
</div>

<script>
function k(key){fetch('/api/key?k='+encodeURIComponent(key),{method:'POST'});}
function esc(s){return s.replace(/[&<>"]/g,c=>({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;'}[c]));}
let misses=0;
async function tick(){
  try{
    const r=await fetch('/api/status',{cache:'no-store'});
    const d=await r.json();
    misses=0;document.getElementById('stale').style.display='none';

    let state='Disarmed',sub='System ready';
    if(d.alarm){state='ALARM';sub='Alarm triggered';}
    else if(d.fire){state='FIRE ALARM';sub='Fire alarm';}
    else if(d.entryDelay){state='Entry delay';sub='Disarm now';}
    else if(d.exitDelay){state='Exit delay';sub='Arming…';}
    else if(d.armed){state='Armed '+(d.armedAway?'away':d.armedStay?'stay':'');
      sub=d.noEntryDelay?'No entry delay':' ';}
    else sub=d.ready?'Ready to arm':'Not ready — a zone is open';
    const s=document.getElementById('state');
    s.textContent=state;
    s.className='state'+((d.alarm||d.fire)?' bad':d.armed?' warn':' good');
    document.getElementById('sub').textContent=sub;

    document.getElementById('zones').innerHTML=d.zones.map(z=>
      `<div class="z"><b>${esc(z.name)}</b><span class="tag ${z.open?'open':'closed'}">`+
      `${z.alarm?'alarm':z.open?'open':'closed'}</span></div>`).join('');

    document.getElementById('sys').innerHTML=[
      ['Keybus',d.keybus?'connected':'disconnected',d.keybus],
      ['AC power',d.acPower?'ok':'trouble',d.acPower],
      ['Battery',d.battery?'ok':'trouble',d.battery],
      ['Trouble',d.trouble?'yes':'none',!d.trouble],
      ['Panel time',d.time||'—',true],
      ['Uptime',d.uptime,true]
    ].map(([l,v,ok])=>`<span>${l}: <b class="${ok?'good':'bad'}">${esc(String(v))}</b></span>`).join('');

    document.getElementById('keypadCard').hidden=!d.keypad;
  }catch(e){ if(++misses>2) document.getElementById('stale').style.display='block'; }
}
tick();setInterval(tick,1500);
</script></div></body></html>)HTML";


// ---------------------------------------------------------------- json ----

static String uptimeString() {
  unsigned long s = millis() / 1000;
  char buf[24];
  snprintf(buf, sizeof(buf), "%lud %luh %lum", s / 86400, (s % 86400) / 3600, (s % 3600) / 60);
  return String(buf);
}

static void handleStatus() {
  String j;
  j.reserve(1024);
  j += '{';

  j += "\"ready\":";       j += dsc.ready[0] ? "true" : "false";
  j += ",\"armed\":";      j += dsc.armed[0] ? "true" : "false";
  j += ",\"armedAway\":";  j += dsc.armedAway[0] ? "true" : "false";
  j += ",\"armedStay\":";  j += dsc.armedStay[0] ? "true" : "false";
  j += ",\"noEntryDelay\":"; j += dsc.noEntryDelay[0] ? "true" : "false";
  j += ",\"alarm\":";      j += dsc.alarm[0] ? "true" : "false";
  j += ",\"fire\":";       j += dsc.fire[0] ? "true" : "false";
  j += ",\"exitDelay\":";  j += dsc.exitDelay[0] ? "true" : "false";
  j += ",\"entryDelay\":"; j += dsc.entryDelay[0] ? "true" : "false";
  j += ",\"keybus\":";     j += dsc.keybusConnected ? "true" : "false";
  j += ",\"acPower\":";    j += dsc.powerTrouble ? "false" : "true";
  j += ",\"battery\":";    j += dsc.batteryTrouble ? "false" : "true";
  j += ",\"trouble\":";    j += dsc.trouble ? "true" : "false";

#ifdef ENABLE_VIRTUAL_KEYPAD
  j += ",\"keypad\":true";
#else
  j += ",\"keypad\":false";
#endif

  j += ",\"uptime\":\"" + uptimeString() + "\"";

  j += ",\"time\":\"";
  if (dsc.year != 0) {
    char t[20];
    snprintf(t, sizeof(t), "%04u.%02u.%02u %02u:%02u",
             (unsigned)dsc.year, (unsigned)dsc.month, (unsigned)dsc.day,
             (unsigned)dsc.hour, (unsigned)dsc.minute);
    j += t;
  }
  j += '"';

  j += ",\"zones\":[";
  for (byte i = 0; i < ZONE_COUNT; i++) {
    if (i) j += ',';
    j += "{\"n\":";     j += (i + 1);
    j += ",\"name\":\""; j += zoneNames[i];
    j += "\",\"open\":"; j += bitRead(dsc.openZones[i / 8], i % 8) ? "true" : "false";
    j += ",\"alarm\":";  j += bitRead(dsc.alarmZones[i / 8], i % 8) ? "true" : "false";
    j += '}';
  }
  j += "]}";

  server.sendHeader("Cache-Control", "no-store");
  server.send(200, "application/json", j);
}

static void handleKey() {
#ifdef ENABLE_VIRTUAL_KEYPAD
  if (!server.hasArg("k")) { server.send(400, "text/plain", "missing k"); return; }
  String k = server.arg("k");
  if (k.length() != 1) { server.send(400, "text/plain", "bad key"); return; }
  dsc.write(k.charAt(0));
  server.send(200, "text/plain", "ok");
#else
  server.send(403, "text/plain", "virtual keypad disabled at build time");
#endif
}


// --------------------------------------------------------------- setup ----

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println();

  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);            // keeps the web UI responsive
  WiFi.setHostname(MDNS_HOSTNAME);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  Serial.print(F("Connecting to WiFi"));
  while (WiFi.status() != WL_CONNECTED) { Serial.print('.'); delay(500); }
  Serial.println();
  Serial.print(F("IP address: http://"));
  Serial.println(WiFi.localIP());

  if (MDNS.begin(MDNS_HOSTNAME)) {
    MDNS.addService("http", "tcp", 80);
    Serial.printf("Also at: http://%s.local/\n", MDNS_HOSTNAME);
  }

  server.on("/", HTTP_GET, []() {
    server.send_P(200, "text/html", INDEX_HTML);
  });
  server.on("/api/status", HTTP_GET, handleStatus);
  server.on("/api/key", HTTP_POST, handleKey);
  server.onNotFound([]() { server.send(404, "text/plain", "not found"); });
  server.begin();

  dsc.begin();
  Serial.println(F("DSC Keybus interface online."));
}


void loop() {
  dsc.loop();                 // must run often — call it before anything slow
  server.handleClient();

  if (dsc.statusChanged) {
    dsc.statusChanged = false;
    if (dsc.bufferOverflow) {
      dsc.bufferOverflow = false;
      Serial.println(F("Keybus buffer overflow"));
    }
    // Clear the per-item change flags; the web UI polls current state instead.
    for (byte p = 0; p < dscPartitions; p++) {
      dsc.readyChanged[p] = dsc.armedChanged[p] = dsc.alarmChanged[p] = false;
      dsc.exitDelayChanged[p] = dsc.entryDelayChanged[p] = false;
      dsc.fireChanged[p] = dsc.disabledChanged[p] = dsc.accessCodeChanged[p] = false;
    }
    dsc.openZonesStatusChanged = false;
    dsc.alarmZonesStatusChanged = false;
    dsc.pgmOutputsStatusChanged = false;
    dsc.keybusChanged = dsc.troubleChanged = false;
    dsc.powerChanged = dsc.batteryChanged = dsc.timestampChanged = false;
  }

  // Reconnect WiFi if it drops
  static unsigned long lastCheck = 0;
  if (millis() - lastCheck > 30000) {
    lastCheck = millis();
    if (WiFi.status() != WL_CONNECTED) {
      Serial.println(F("WiFi lost, reconnecting"));
      WiFi.disconnect();
      WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    }
  }
}

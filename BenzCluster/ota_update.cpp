#include "ota_update.h"
#include <Arduino.h>
#include <ArduinoOTA.h>
#include <WebServer.h>
#include <WiFi.h>

static WebServer *webServer = nullptr;

// HTML Dashboard page
static const char DASHBOARD_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Star Trail Dashboard</title>
<style>
*{margin:0;padding:0;box-sizing:border-box}
body{font-family:'Segoe UI',sans-serif;background:#0a0a1a;color:#fff;min-height:100vh;display:flex;flex-direction:column;align-items:center;padding:20px}
h1{font-size:1.5em;color:#00ccff;letter-spacing:3px;margin-bottom:20px}
.card{background:#141428;border-radius:12px;padding:16px;margin:8px 0;width:100%;max-width:400px;border:1px solid #222244}
.card h2{font-size:0.9em;color:#888;margin-bottom:8px;text-transform:uppercase;letter-spacing:2px}
.row{display:flex;justify-content:space-between;padding:4px 0}
.label{color:#666}
.value{color:#00ff88;font-family:monospace;font-size:1.1em}
.btn{background:#00ccff;color:#000;border:none;padding:10px 24px;border-radius:8px;cursor:pointer;font-weight:bold;margin:4px;font-size:0.9em}
.btn:hover{background:#00aadd}
.btn-danger{background:#ff4444;color:#fff}
.btn-danger:hover{background:#cc2222}
.status{display:inline-block;width:8px;height:8px;border-radius:50%;margin-right:6px}
.ok{background:#00ff88}
.warn{background:#ffaa00}
.fail{background:#ff4444}
#log{background:#0a0a12;border:1px solid #222;border-radius:8px;padding:10px;font-family:monospace;font-size:0.75em;height:200px;overflow-y:auto;color:#888;white-space:pre-wrap;width:100%;max-width:400px;margin-top:8px}
</style>
</head>
<body>
<h1>★ STAR TRAIL</h1>
<div class="card">
<h2>Sensors</h2>
<div class="row"><span class="label">Heading</span><span class="value" id="h">--</span></div>
<div class="row"><span class="label">Pitch</span><span class="value" id="p">--</span></div>
<div class="row"><span class="label">Roll</span><span class="value" id="r">--</span></div>
<div class="row"><span class="label">Temperature</span><span class="value" id="t">--</span></div>
<div class="row"><span class="label">Altitude</span><span class="value" id="a">--</span></div>
<div class="row"><span class="label">Mag Raw</span><span class="value" id="m">--</span></div>
</div>
<div class="card">
<h2>System</h2>
<div class="row"><span class="label">WiFi</span><span class="value" id="ip">--</span></div>
<div class="row"><span class="label">Uptime</span><span class="value" id="up">--</span></div>
<div class="row"><span class="label">Free Heap</span><span class="value" id="heap">--</span></div>
</div>
<div class="card">
<h2>Actions</h2>
<button class="btn" onclick="fetch('/calibrate')">Calibrate Mag</button>
<button class="btn btn-danger" onclick="if(confirm('Reboot?'))fetch('/reboot')">Reboot</button>
</div>
<div id="log">Connecting...</div>
<script>
function poll(){
fetch('/api/status').then(r=>r.json()).then(d=>{
document.getElementById('h').textContent=d.heading.toFixed(0)+'°';
document.getElementById('p').textContent=d.pitch.toFixed(1)+'°';
document.getElementById('r').textContent=d.roll.toFixed(1)+'°';
document.getElementById('t').textContent=d.temp.toFixed(1)+'°C';
document.getElementById('a').textContent=d.alt.toFixed(0)+' ft';
document.getElementById('m').textContent=d.mx+','+d.my+','+d.mz;
document.getElementById('ip').textContent=d.ip;
document.getElementById('up').textContent=Math.floor(d.uptime/60)+'m';
document.getElementById('heap').textContent=Math.floor(d.heap/1024)+'KB';
document.getElementById('log').textContent='Last update: '+new Date().toLocaleTimeString()+'\n'+JSON.stringify(d,null,1);
}).catch(e=>{document.getElementById('log').textContent='Error: '+e;});
}
poll();setInterval(poll,1000);
</script>
</body>
</html>
)rawliteral";

// Forward declarations for sensor data
extern volatile float gH, gP, gR;
extern volatile float gT, gA, gPr;
extern bool calibrationMode;

void ota_init() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[OTA] WiFi not connected, OTA disabled");
    return;
  }

  // ArduinoOTA
  ArduinoOTA.setHostname("StarTrail");
  ArduinoOTA.onStart([]() {
    String type = (ArduinoOTA.getCommand() == U_FLASH) ? "Firmware" : "SPIFFS";
    Serial.printf("[OTA] Start updating %s\n", type.c_str());
  });
  ArduinoOTA.onEnd(
      []() { Serial.println("\n[OTA] Update complete! Rebooting..."); });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("[OTA] Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("[OTA] Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR)
      Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR)
      Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR)
      Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR)
      Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR)
      Serial.println("End Failed");
  });
  ArduinoOTA.begin();

  // Web Dashboard
  webServer = new WebServer(80);

  webServer->on("/", HTTP_GET,
                []() { webServer->send_P(200, "text/html", DASHBOARD_HTML); });

  webServer->on("/api/status", HTTP_GET, []() {
    int16_t mx = 0, my = 0, mz = 0;
    // Try to get mag raw if available
    extern void sensors_get_mag_raw(int16_t *, int16_t *, int16_t *);
    sensors_get_mag_raw(&mx, &my, &mz);

    char json[256];
    snprintf(json, sizeof(json),
             "{\"heading\":%.1f,\"pitch\":%.1f,\"roll\":%.1f,\"temp\":%.1f,"
             "\"alt\":%.1f,"
             "\"pressure\":%.1f,\"mx\":%d,\"my\":%d,\"mz\":%d,"
             "\"ip\":\"%s\",\"uptime\":%lu,\"heap\":%u}",
             (float)gH, (float)gP, (float)gR, (float)gT, (float)gA, (float)gPr,
             mx, my, mz, WiFi.localIP().toString().c_str(), millis() / 1000,
             ESP.getFreeHeap());
    webServer->send(200, "application/json", json);
  });

  webServer->on("/calibrate", HTTP_GET, []() {
    calibrationMode = true;
    extern void calibration_start();
    calibration_start();
    webServer->send(200, "text/plain", "Calibration started (30s)");
    Serial.println("[WEB] Remote calibration triggered");
  });

  webServer->on("/reboot", HTTP_GET, []() {
    webServer->send(200, "text/plain", "Rebooting...");
    delay(500);
    ESP.restart();
  });

  webServer->begin();
  Serial.printf("[WEB] Dashboard at http://%s/\n",
                WiFi.localIP().toString().c_str());
  Serial.printf("[OTA] Ready! Use 'StarTrail.local' or IP %s\n",
                WiFi.localIP().toString().c_str());
}

void ota_handle() {
  ArduinoOTA.handle();
  if (webServer)
    webServer->handleClient();
}

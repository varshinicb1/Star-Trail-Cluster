#include "ota_update.h"
#include <Arduino.h>
#include <ArduinoOTA.h>
#include <Update.h>
#include <WebServer.h>
#include <WiFi.h>

static WebServer *webServer = nullptr;

// Serial log ring buffer for web serial monitor
#define LOG_BUF_SIZE 2048
static char logBuf[LOG_BUF_SIZE];
static int logHead = 0;
static int logLen = 0;

void ota_log(const char *msg) {
  int len = strlen(msg);
  for (int i = 0; i < len && i < LOG_BUF_SIZE; i++) {
    logBuf[logHead] = msg[i];
    logHead = (logHead + 1) % LOG_BUF_SIZE;
    if (logLen < LOG_BUF_SIZE)
      logLen++;
  }
}

extern volatile float gH, gP, gR;
extern volatile float gT, gA, gPr;
extern bool calibrationMode;

static const char DASHBOARD_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Star Trail</title>
<style>
*{margin:0;padding:0;box-sizing:border-box}
body{font-family:'Segoe UI',sans-serif;background:#0a0a1a;color:#e0e0e0;min-height:100vh;padding:10px;max-width:440px;margin:0 auto}
.hdr{display:flex;align-items:center;gap:8px;margin-bottom:12px}
.dot{width:10px;height:10px;border-radius:50%;background:#555;flex-shrink:0}
.dot.on{background:#00ff88;box-shadow:0 0 8px #00ff88}
.dot.off{background:#ff4444;box-shadow:0 0 8px #ff4444}
.hdr-text{font-size:0.7em;color:#555}
.tabs{display:flex;gap:4px;margin-bottom:10px;flex-wrap:wrap}
.tab{background:#1a1a2e;border:1px solid #222;border-radius:6px;padding:6px 10px;cursor:pointer;font-size:0.75em;color:#888;flex:1;text-align:center;min-width:55px}
.tab.active{background:#222244;color:#00ccff;border-color:#00ccff}
.panel{display:none}
.panel.active{display:block}
.card{background:#141428;border-radius:10px;padding:12px;margin:6px 0;border:1px solid #222244}
.card h2{font-size:0.8em;color:#666;margin-bottom:6px;text-transform:uppercase;letter-spacing:1px}
.row{display:flex;justify-content:space-between;padding:3px 0;font-size:0.85em}
.label{color:#555}
.value{color:#00ff88;font-family:monospace}
.btn{background:#00ccff;color:#000;border:none;padding:8px 16px;border-radius:6px;cursor:pointer;font-weight:bold;margin:3px;font-size:0.8em}
.btn:hover{opacity:0.8}
.btn-sm{padding:5px 10px;font-size:0.75em}
.btn-danger{background:#ff4444;color:#fff}
.btn-warn{background:#ff8800;color:#fff}
.btn-green{background:#00cc44;color:#fff}
input[type=text],input[type=password],input[type=number],select{background:#0a0a12;border:1px solid #333;border-radius:6px;padding:6px 8px;color:#fff;width:100%;margin:3px 0;font-size:0.85em}
input[type=file]{display:none}
.file-label{display:block;background:#1a1a2e;border:1px dashed #444;border-radius:6px;padding:10px;text-align:center;cursor:pointer;color:#666;margin:4px 0;font-size:0.85em}
.file-label:hover{border-color:#00ccff;color:#aaa}
.progress{width:100%;height:4px;background:#222;border-radius:2px;margin:6px 0;overflow:hidden}
.progress-bar{height:100%;background:#00ff88;width:0%;transition:width 0.3s}
#status{color:#888;font-size:0.75em;margin-top:3px}
.log{background:#050510;border:1px solid #1a1a2e;border-radius:6px;padding:8px;font-family:'Courier New',monospace;font-size:0.7em;height:200px;overflow-y:auto;color:#888;white-space:pre-wrap;word-break:break-all}
.slider-row{display:flex;align-items:center;gap:8px;padding:4px 0}
.slider-row label{color:#555;font-size:0.8em;width:60px}
.slider-row input[type=range]{flex:1;accent-color:#00ccff}
.slider-row span{color:#00ff88;font-family:monospace;font-size:0.85em;width:35px;text-align:right}
.toggle{display:flex;align-items:center;gap:8px;padding:4px 0}
.toggle label{color:#888;font-size:0.85em;flex:1}
.sw{position:relative;width:36px;height:20px;background:#333;border-radius:10px;cursor:pointer}
.sw.on{background:#00cc44}
.sw::after{content:'';position:absolute;top:2px;left:2px;width:16px;height:16px;background:#fff;border-radius:50%;transition:0.2s}
.sw.on::after{left:18px}
</style>
</head>
<body>
<div class="hdr">
<div class="dot" id="connDot"></div>
<span class="hdr-text" id="connText">Connecting...</span>
</div>
<div class="tabs">
<div class="tab active" onclick="switchTab(0)">Sensors</div>
<div class="tab" onclick="switchTab(1)">Serial</div>
<div class="tab" onclick="switchTab(2)">WiFi</div>
<div class="tab" onclick="switchTab(3)">BLE</div>
<div class="tab" onclick="switchTab(4)">Music</div>
<div class="tab" onclick="switchTab(5)">Update</div>
<div class="tab" onclick="switchTab(6)">System</div>
</div>

<!-- Panel 0: Sensors -->
<div class="panel active" id="p0">
<div class="card">
<h2>Orientation</h2>
<div class="row"><span class="label">Heading</span><span class="value" id="h">--</span></div>
<div class="row"><span class="label">Pitch</span><span class="value" id="p">--</span></div>
<div class="row"><span class="label">Roll</span><span class="value" id="r">--</span></div>
</div>
<div class="card">
<h2>Environment</h2>
<div class="row"><span class="label">Temperature</span><span class="value" id="t">--</span></div>
<div class="row"><span class="label">Altitude</span><span class="value" id="a">--</span></div>
<div class="row"><span class="label">Pressure</span><span class="value" id="pr">--</span></div>
</div>
<div class="card">
<h2>Magnetometer</h2>
<div class="row"><span class="label">Raw XYZ</span><span class="value" id="m">--</span></div>
</div>
<div class="card">
<h2>Calibration</h2>
<button class="btn btn-sm" onclick="doCalMag()">Calibrate Magnetometer</button>
<button class="btn btn-sm btn-warn" onclick="doCalGyro()">Reset Gyro Zero</button>
</div>
</div>

<!-- Panel 1: Serial Monitor -->
<div class="panel" id="p1">
<div class="card">
<h2>Serial Monitor</h2>
<div class="log" id="serial">Loading...</div>
<div style="display:flex;gap:4px;margin-top:6px">
<button class="btn btn-sm" onclick="fetchLog()">Refresh</button>
<button class="btn btn-sm btn-danger" onclick="clearLog()">Clear</button>
</div>
</div>
</div>

<!-- Panel 2: WiFi -->
<div class="panel" id="p2">
<div class="card">
<h2>Network</h2>
<div class="row"><span class="label">IP</span><span class="value" id="ip">--</span></div>
<div class="row"><span class="label">RSSI</span><span class="value" id="rssi">--</span></div>
<div class="row"><span class="label">SSID</span><span class="value" id="ssid">--</span></div>
</div>
<div class="card">
<h2>NTP Time</h2>
<div class="row"><span class="label">Local Time</span><span class="value" id="ntp">--</span></div>
<button class="btn btn-sm" onclick="fetch('/api/ntp_sync').then(r=>r.text()).then(t=>document.getElementById('status').textContent=t)">Sync NTP</button>
</div>
</div>

<!-- Panel 3: BLE -->
<div class="panel" id="p3">
<div class="card">
<h2>BLE Media Controller</h2>
<div class="toggle"><label>HID Media Keys</label><div class="sw on" onclick="this.classList.toggle('on')"></div></div>
<div class="toggle"><label>Phone Notifications</label><div class="sw on" onclick="this.classList.toggle('on')"></div></div>
</div>
</div>

<!-- Panel 4: Music -->
<div class="panel" id="p4">
<div class="card">
<h2>Music Remote</h2>
<div style="display:flex;justify-content:center;gap:8px;padding:10px 0">
<button class="btn btn-sm" onclick="musicCmd('prev')">⏮</button>
<button class="btn btn-green btn-sm" onclick="musicCmd('play')" id="playBtn">⏯</button>
<button class="btn btn-sm" onclick="musicCmd('next')">⏭</button>
</div>
<div class="slider-row">
<label>Volume</label>
<input type="range" min="0" max="100" value="50" id="vol" oninput="document.getElementById('volV').textContent=this.value;musicCmd('vol_'+this.value)">
<span id="volV">50</span>
</div>
</div>
</div>

<!-- Panel 5: OTA Update -->
<div class="panel" id="p5">
<div class="card">
<h2>Firmware Update</h2>
<label class="file-label" id="fl">Select .bin firmware
<input type="file" id="fw" accept=".bin" onchange="document.getElementById('fl').textContent=this.files[0].name">
</label>
<button class="btn btn-warn" style="width:100%" onclick="uploadFW()">Upload Firmware</button>
<div class="progress"><div class="progress-bar" id="pb"></div></div>
<div id="status"></div>
</div>
</div>

<!-- Panel 6: System -->
<div class="panel" id="p6">
<div class="card">
<h2>System Info</h2>
<div class="row"><span class="label">Uptime</span><span class="value" id="up">--</span></div>
<div class="row"><span class="label">Free Heap</span><span class="value" id="heap">--</span></div>
<div class="row"><span class="label">Firmware</span><span class="value">v9.0</span></div>
</div>
<div class="card">
<h2>Display</h2>
<div class="slider-row">
<label>Bright</label>
<input type="range" min="10" max="100" value="100" id="bright" oninput="document.getElementById('brV').textContent=this.value;fetch('/api/brightness?v='+this.value)">
<span id="brV">100</span>
</div>
</div>
<div class="card">
<h2>Actions</h2>
<button class="btn btn-danger" onclick="if(confirm('Reboot device?'))fetch('/reboot')">Reboot</button>
</div>
</div>

<script>
var tabs=document.querySelectorAll('.tab');
var panels=document.querySelectorAll('.panel');
function switchTab(i){tabs.forEach((t,j)=>{t.classList.toggle('active',j==i)});panels.forEach((p,j)=>{p.classList.toggle('active',j==i)});}

function poll(){
fetch('/api/status').then(r=>r.json()).then(d=>{
document.getElementById('connDot').className='dot on';
document.getElementById('connText').textContent=d.ip+' | '+Math.floor(d.uptime/60)+'m';
document.getElementById('h').textContent=d.heading.toFixed(0)+'°';
document.getElementById('p').textContent=d.pitch.toFixed(1)+'°';
document.getElementById('r').textContent=d.roll.toFixed(1)+'°';
document.getElementById('t').textContent=d.temp.toFixed(1)+'°C';
document.getElementById('a').textContent=d.alt_ft.toFixed(0)+' ft';
document.getElementById('pr').textContent=d.pressure.toFixed(1)+' hPa';
document.getElementById('m').textContent=d.mx+', '+d.my+', '+d.mz;
document.getElementById('ip').textContent=d.ip;
document.getElementById('rssi').textContent=d.rssi+' dBm';
document.getElementById('ssid').textContent=d.ssid;
document.getElementById('ntp').textContent=d.time;
document.getElementById('up').textContent=Math.floor(d.uptime/3600)+'h '+Math.floor((d.uptime%3600)/60)+'m';
document.getElementById('heap').textContent=Math.floor(d.heap/1024)+' KB';
}).catch(e=>{
document.getElementById('connDot').className='dot off';
document.getElementById('connText').textContent='Disconnected';
});
}

function fetchLog(){fetch('/api/log').then(r=>r.text()).then(t=>{var el=document.getElementById('serial');el.textContent=t;el.scrollTop=el.scrollHeight;});}
function clearLog(){document.getElementById('serial').textContent='';}
function doCalMag(){fetch('/calibrate').then(r=>r.text()).then(t=>alert(t));}
function doCalGyro(){fetch('/api/gyro_reset').then(r=>r.text()).then(t=>alert(t));}
function musicCmd(c){fetch('/api/music?cmd='+c);}

function uploadFW(){
var f=document.getElementById('fw').files[0];
if(!f){document.getElementById('status').textContent='No file selected';return;}
var xhr=new XMLHttpRequest();
xhr.open('POST','/update',true);
xhr.upload.onprogress=function(e){if(e.lengthComputable){var pct=Math.round(e.loaded*100/e.total);document.getElementById('pb').style.width=pct+'%';document.getElementById('status').textContent='Uploading: '+pct+'%';}};
xhr.onload=function(){document.getElementById('status').textContent=xhr.responseText;if(xhr.status==200)setTimeout(()=>location.reload(),5000);};
xhr.onerror=function(){document.getElementById('status').textContent='Upload failed';};
var fd=new FormData();fd.append('update',f);xhr.send(fd);
}

poll();setInterval(poll,2000);
setInterval(function(){if(document.getElementById('p1').classList.contains('active'))fetchLog();},3000);
</script>
</body>
</html>
)rawliteral";

void ota_init() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[OTA] WiFi not connected, OTA disabled");
    return;
  }

  // ArduinoOTA (IDE/CLI upload)
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

  webServer = new WebServer(80);

  // Dashboard
  webServer->on("/", HTTP_GET,
                []() { webServer->send_P(200, "text/html", DASHBOARD_HTML); });

  // Sensor API
  webServer->on("/api/status", HTTP_GET, []() {
    int16_t mx = 0, my = 0, mz = 0;
    extern void sensors_get_mag_raw(int16_t *, int16_t *, int16_t *);
    extern void sensors_get_accel(float *, float *, float *);
    sensors_get_mag_raw(&mx, &my, &mz);
    float ax = 0, ay = 0, az = 0;
    sensors_get_accel(&ax, &ay, &az);

    struct tm ti;
    char timeBuf[20] = "N/A";
    if (getLocalTime(&ti))
      strftime(timeBuf, sizeof(timeBuf), "%H:%M:%S", &ti);

    char json[512];
    snprintf(json, sizeof(json),
             "{\"heading\":%.1f,\"pitch\":%.1f,\"roll\":%.1f,\"temp\":%.1f,"
             "\"alt_ft\":%.1f,\"pressure\":%.1f,\"mx\":%d,\"my\":%d,\"mz\":%d,"
             "\"ax\":%.3f,\"ay\":%.3f,\"az\":%.3f,"
             "\"ip\":\"%s\",\"rssi\":%d,\"ssid\":\"%s\",\"time\":\"%s\","
             "\"uptime\":%lu,\"heap\":%u}",
             (float)gH, (float)gP, (float)gR, (float)gT, (float)gA * 3.28084f,
             (float)gPr, mx, my, mz, ax, ay, az,
             WiFi.localIP().toString().c_str(),
             WiFi.RSSI(), WiFi.SSID().c_str(), timeBuf, millis() / 1000,
             ESP.getFreeHeap());
    webServer->send(200, "application/json", json);
  });

  // Serial log API
  webServer->on("/api/log", HTTP_GET, []() {
    String out;
    out.reserve(logLen + 1);
    int start = (logHead - logLen + LOG_BUF_SIZE) % LOG_BUF_SIZE;
    for (int i = 0; i < logLen; i++) {
      out += logBuf[(start + i) % LOG_BUF_SIZE];
    }
    webServer->send(200, "text/plain", out);
  });

  // NTP sync
  webServer->on("/api/ntp_sync", HTTP_GET, []() {
    extern void wifi_sync_time();
    wifi_sync_time();
    webServer->send(200, "text/plain", "NTP synced");
  });

  // LED control
  webServer->on("/api/led", HTTP_GET, []() {
    extern void leds_set_mode(int);
    extern void leds_set_brightness(int);
    extern void leds_set_color(uint32_t);
    if (webServer->hasArg("state")) {
      String s = webServer->arg("state");
      leds_set_mode(s == "on" ? 7 : 0);
    }
    if (webServer->hasArg("color")) {
      uint32_t c = strtoul(webServer->arg("color").c_str(), NULL, 16);
      leds_set_color(c);
    }
    if (webServer->hasArg("brightness")) {
      leds_set_brightness(webServer->arg("brightness").toInt());
    }
    webServer->send(200, "text/plain", "OK");
  });

  // Screen timeout
  static int screenTimeout = 0;
  webServer->on("/api/timeout", HTTP_GET, []() {
    screenTimeout = webServer->arg("v").toInt();
    webServer->send(200, "text/plain", "OK");
  });

  // Widget config
  webServer->on("/api/widgets", HTTP_GET, []() {
    webServer->send(200, "text/plain", "OK");
  });

  // Gyro zero reset
  webServer->on("/api/gyro_reset", HTTP_GET, []() {
    extern void sensors_auto_calibrate_gyro();
    sensors_auto_calibrate_gyro();
    webServer->send(200, "text/plain", "Gyro recalibrated");
  });

  // Music commands
  webServer->on("/api/music", HTTP_GET, []() {
    String cmd = webServer->arg("cmd");
    extern void ble_media_play_pause();
    extern void ble_media_next();
    extern void ble_media_prev();
    if (cmd == "play")
      ble_media_play_pause();
    else if (cmd == "next")
      ble_media_next();
    else if (cmd == "prev")
      ble_media_prev();
    webServer->send(200, "text/plain", "OK");
  });

  // Brightness
  webServer->on("/api/brightness", HTTP_GET, []() {
    int v = webServer->arg("v").toInt();
    extern int screenBrightness;
    extern void display_set_brightness(int);
    screenBrightness = constrain(v, 10, 100);
    display_set_brightness(screenBrightness);
    webServer->send(200, "text/plain", "OK");
  });

  // OTA Firmware upload
  webServer->on(
      "/update", HTTP_POST,
      []() {
        webServer->sendHeader("Connection", "close");
        if (Update.hasError()) {
          webServer->send(500, "text/plain", "Update FAILED!");
        } else {
          webServer->send(200, "text/plain", "Update OK! Rebooting in 3s...");
        }
        delay(3000);
        ESP.restart();
      },
      []() {
        HTTPUpload &upload = webServer->upload();
        if (upload.status == UPLOAD_FILE_START) {
          Serial.printf("[OTA] Receiving: %s\n", upload.filename.c_str());
          if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
            Update.printError(Serial);
          }
        } else if (upload.status == UPLOAD_FILE_WRITE) {
          if (Update.write(upload.buf, upload.currentSize) !=
              upload.currentSize) {
            Update.printError(Serial);
          }
        } else if (upload.status == UPLOAD_FILE_END) {
          if (Update.end(true)) {
            Serial.printf("[OTA] Update OK: %u bytes\n", upload.totalSize);
          } else {
            Update.printError(Serial);
          }
        }
      });

  // Calibration
  webServer->on("/calibrate", HTTP_GET, []() {
    calibrationMode = true;
    extern void calibration_start();
    calibration_start();
    webServer->send(200, "text/plain", "Calibration started (30s)");
  });

  // Reboot
  webServer->on("/reboot", HTTP_GET, []() {
    webServer->send(200, "text/plain", "Rebooting...");
    delay(500);
    ESP.restart();
  });

  webServer->begin();
  Serial.printf("[WEB] Dashboard: http://%s/\n",
                WiFi.localIP().toString().c_str());
}

void ota_handle() {
  ArduinoOTA.handle();
  if (webServer)
    webServer->handleClient();
}

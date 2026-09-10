#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <esp_task_wdt.h>
#include <esp_timer.h>
#include <Wire.h>
#include <Adafruit_SHT4x.h>
#include <esp_sntp.h>
#include <esp_chip_info.h>

// MQTT
#include <PubSubClient.h>
#include <WiFiClientSecure.h>

#define SCL_PIN 24
#define SDA_PIN 23

// Serie SHT
Adafruit_SHT4x sht4 = Adafruit_SHT4x();

// Mutex
SemaphoreHandle_t shtMutex;

//----------------------------------
// MQTT
//----------------------------------

const char *mqtt_server = "vps67545.publiccloud.com.br";
const char *mqtt_topic = "sensores/esp32/sht45-103";
const char *client_id = "ESP32-SHT45-103";
const char *mqtt_user = "usr_esp32_103";
const char *mqtt_password = "@FlakE2028@#$";

const char *mqtt_ca = R"EOF( 
-----BEGIN CERTIFICATE----- 
MIICizCCAhGgAwIBAgIQXd1w3TH4AchcGGp6BLgK/jAKBggqhkjOPQQDAzAuMQsw
CQYDVQQGEwJVUzENMAsGA1UEChMESVNSRzEQMA4GA1UEAxMHUm9vdCBZRTAeFw0y
NTA5MDMwMDAwMDBaFw0yODA5MDIyMzU5NTlaMDMxCzAJBgNVBAYTAlVTMRYwFAYD
VQQKEw1MZXQncyBFbmNyeXB0MQwwCgYDVQQDEwNZRTEwdjAQBgcqhkjOPQIBBgUr
gQQAIgNiAAQHZVB1/mimla2hfSurylScjPMZaOJXLz/NnAc2sylm8WDyhU9Ccp+z
ASQi5vSwGGJjSGklkD9fdPR8GpyDIOIjCEfrnbt/v+ZSEPLLEGbaM6EccDbN7p9x
teIm2Avf+ryjge4wgeswDgYDVR0PAQH/BAQDAgGGMBMGA1UdJQQMMAoGCCsGAQUF
BwMBMBIGA1UdEwEB/wQIMAYBAf8CAQAwHQYDVR0OBBYEFLsgykcL/tflnPmPCSqj
jDdFsbzYMB8GA1UdIwQYMBaAFKPIJlqOoUzQNWP8myPIOq5W809WMDIGCCsGAQUF
BwEBBCYwJDAiBggrBgEFBQcwAoYWaHR0cDovL3llLmkubGVuY3Iub3JnLzATBgNV
HSAEDDAKMAgGBmeBDAECATAnBgNVHR8EIDAeMBygGqAYhhZodHRwOi8veWUuYy5s
ZW5jci5vcmcvMAoGCCqGSM49BAMDA2gAMGUCMQDgjUEahFT/h3DRakqiPZpLvPgf
Zwkt6K2EOMmh1nvEzl83eMLYcod4GCl3b0J1Nn0CMBNYmEQJb4CEG5WoOe7aRn/L
VKu6saHmHEynI7ysIPd8zQsK1HdmhlHKlw9Z5GpGvA==
-----END CERTIFICATE----- 
)EOF";

WiFiClientSecure espClient;
PubSubClient client(espClient);

unsigned long lastMqttPublish = 0;

//----------------------------------
// Wi-Fi
//----------------------------------

const char *wifi_hostname = "ESP32WEBSERVER";
const char *ssid = "GABRIEL_HOME_5G";
const char *wifi_password = "@FlakE2021#";

// const char *ssid = "POCOX6";
// const char *password = "int30int";

// const char *ssid = "CAMILA";
// const char *password = "@Mccg205..";

//----------------------------------
// IP estático
//----------------------------------

IPAddress local_IP(192, 168, 1, 103);
// IPAddress local_IP(192, 168, 0, 103); // GOIANIA

IPAddress gateway(192, 168, 1, 1);
// IPAddress gateway(192, 168, 0, 1); // GOIANIA

IPAddress subnet(255, 255, 255, 0);
IPAddress primaryDNS(8, 8, 8, 8);
IPAddress secondaryDNS(8, 8, 4, 4);

//----------------------------------
// Web Server / WebSockets
//----------------------------------

AsyncWebServer server(80);

// WebSocket existente: monitoramento de hardware
AsyncWebSocket ws("/ws");

// Novo WebSocket: exclusivamente SHT45
AsyncWebSocket wsSht45("/ws/sht45");

unsigned long lastPrint = 0;

static unsigned long lastNtpSync = 0;

//----------------------------------
// CACHE SHT45
//----------------------------------

struct Sht45Data {
  float temperaturaCelsius = NAN;
  float temperaturaFahrenheit = NAN;
  float umidade = NAN;
  String dataHora = "";
  String uptime = "";
  String sensorIp = "";
  int rssi = 0;
  bool valido = false;
};

Sht45Data sht45Data;

unsigned long lastSht45Read = 0;

const unsigned long SHT45_READ_INTERVAL = 2000;

//----------------------------------
// Declarações antecipadas
//----------------------------------

void connectWiFi();

void handleRoot(AsyncWebServerRequest *request);

void handleJSON(AsyncWebServerRequest *request);

void syncTime();

void reconnectMQTT();

void publishSensorData();

String getCurrentDateTime(int attempts = 4);

String getUptime();

bool tryReadSensor(
  float &temperatureCelsius,
  float &temperatureFahrenheit,
  float &humidity,
  bool origem);

String generateJSON();

String generateSht45JSON();

bool updateSht45Data();

//------------------------------------------------
// WebSocket hardware
//------------------------------------------------

void sendSystemInfo() {
  String json = generateJSON();
  ws.textAll(json);
}

//------------------------------------------------
// Data / Hora
//------------------------------------------------

String getCurrentDateTime(int attempts) {

  String dateTime =
    "❌ Erro ao obter data e hora";

  while (attempts-- > 0) {

    struct tm timeinfo;

    if (getLocalTime(&timeinfo)) {

      char buffer[20];

      strftime(
        buffer,
        sizeof(buffer),
        "%Y-%m-%d %H:%M:%S",
        &timeinfo);

      dateTime = String(buffer);

      break;
    }

    delay(1000);
  }

  return dateTime;
}

//------------------------------------------------
// SHT45
//------------------------------------------------

bool tryReadSensor(
  float &temperatureCelsius,
  float &temperatureFahrenheit,
  float &humidity,
  bool origem) {

  while (true) {

    if (
      xSemaphoreTake(
        shtMutex,
        portMAX_DELAY)
      == pdTRUE) {

      sensors_event_t humidityEvent;
      sensors_event_t tempEvent;

      sht4.getEvent(
        &humidityEvent,
        &tempEvent);

      temperatureCelsius = tempEvent.temperature;

      humidity = humidityEvent.relative_humidity;

      bool valido = !isnan(temperatureCelsius) && !isnan(humidity) && temperatureCelsius > 10.0 && temperatureCelsius < 50.0;

      xSemaphoreGive(shtMutex);

      if (valido) {
        temperatureFahrenheit =
          temperatureCelsius * 1.8 + 32.0;
        return true;
      }

      if (origem) {
        Serial.println(
          "❌ Falha ao ler SHT45. "
          "Tentando novamente...");
      }
    }
    vTaskDelay(pdMS_TO_TICKS(50));
  }
}

//------------------------------------------------
// Atualiza cache do SHT45
//------------------------------------------------

bool updateSht45Data() {

  float temperatureCelsius;
  float temperatureFahrenheit;
  float humidity;

  if (
    !tryReadSensor(
      temperatureCelsius,
      temperatureFahrenheit,
      humidity,
      true)) {

    sht45Data.valido = false;

    return false;
  }

  sht45Data.temperaturaCelsius = temperatureCelsius;
  sht45Data.temperaturaFahrenheit = temperatureFahrenheit;
  sht45Data.umidade = humidity;
  sht45Data.dataHora = getCurrentDateTime();
  sht45Data.uptime = getUptime();
  sht45Data.sensorIp = WiFi.localIP().toString();
  sht45Data.rssi = WiFi.RSSI();
  sht45Data.valido = true;

  return true;
}

//------------------------------------------------
// JSON exclusivo SHT45
//------------------------------------------------

String generateSht45JSON() {

  StaticJsonDocument<384> doc;

  doc["temperatura_celsius"] = sht45Data.temperaturaCelsius;
  doc["temperatura_fahrenheit"] = sht45Data.temperaturaFahrenheit;
  doc["umidade"] = sht45Data.umidade;
  doc["data_hora"] = sht45Data.dataHora;
  doc["uptime"] = sht45Data.uptime;
  doc["sensor_ip"] = sht45Data.sensorIp;
  doc["rssi"] = sht45Data.rssi;

  String output;

  serializeJson(doc, output);

  return output;
}

//------------------------------------------------
// Uptime
//------------------------------------------------

String getUptime() {

  uint64_t us = esp_timer_get_time();
  uint64_t s = us / 1000000;
  uint32_t sec = s % 60;
  uint32_t min = (s / 60) % 60;
  uint32_t hr = (s / 3600) % 24;
  uint32_t days = s / 86400;

  char buffer[32];

  sprintf(
    buffer,
    "%u:%02u:%02u:%02u",
    days,
    hr,
    min,
    sec);

  return String(buffer);
}

//------------------------------------------------
// TESTE DE PORTA DO MQTT
//------------------------------------------------

void testarMQTTPorta() {

  // Serial.println();
  // Serial.println("================================");
  // Serial.println("TESTE TCP MQTT");
  // Serial.println("================================");

  WiFiClient testClient;

  Serial.print("⌛ Conectando em MQTT broker: ");
  Serial.print(mqtt_server);
  Serial.println(":8883");

  if (testClient.connect(mqtt_server, 8883)) {

    Serial.println("✅ TCP 8883 ACESSÍVEL");

    testClient.stop();

  } else {

    Serial.println("❌ TCP 8883 NÃO ACESSÍVEL");
  }
}

//------------------------------------------------
// SETUP()
//------------------------------------------------

void setup() {

  Serial.begin(115200);

  shtMutex = xSemaphoreCreateMutex();

  if (shtMutex == NULL) {
    Serial.println("❌ Falha ao criar mutex do SHT45");
    while (1);
  }

  Wire.setTimeOut(50);

  Wire.begin(SDA_PIN, SCL_PIN);

  if (!sht4.begin()) {
    Serial.println("❌ SHT45 não encontrado");
    while (1);
  }

  sht4.setPrecision(SHT4X_HIGH_PRECISION);

  sht4.setHeater(SHT4X_NO_HEATER);

  Serial.println();
  Serial.println("✅ Sensor SHT45 iniciado com sucesso");

  esp_task_wdt_config_t wdt_config = {
    .timeout_ms = 240000,
    .idle_core_mask = 0,
    .trigger_panic = true
  };

  esp_task_wdt_reconfigure(&wdt_config);
  esp_task_wdt_add(NULL);

  connectWiFi();

  testarMQTTPorta();

  // MQTT
  espClient.setCACert(mqtt_ca);

  client.setServer(mqtt_server, 8883);

  syncTime();

  //----------------------------------
  // WebSocket hardware existente
  //----------------------------------

  ws.onEvent(
    [](
      AsyncWebSocket *server,
      AsyncWebSocketClient *client,
      AwsEventType type,
      void *arg,
      uint8_t *data,
      size_t len) {});

  server.addHandler(&ws);

  //----------------------------------
  // WebSocket SHT45
  //----------------------------------

  wsSht45.onEvent(
    [](
      AsyncWebSocket *server,
      AsyncWebSocketClient *client,
      AwsEventType type,
      void *arg,
      uint8_t *data,
      size_t len) {
      if (type == WS_EVT_CONNECT) {

        Serial.println(
          "🌡️ WebSocket SHT45 conectado. "
          "Cliente: "
          + String(client->id()));

        if (sht45Data.valido) {

          client->text(
            generateSht45JSON());
        }

      } else if (
        type == WS_EVT_DISCONNECT) {

        Serial.println(
          "🌡️ WebSocket SHT45 desconectado. "
          "Cliente: "
          + String(client->id()));

      } else if (
        type == WS_EVT_ERROR) {

        Serial.println(
          "❌ Erro no WebSocket SHT45");
      }
    });

  server.addHandler(
    &wsSht45);

  //----------------------------------
  // API temperatura existente
  //----------------------------------

  server.on(
    "/esp32/api/temperatura",
    HTTP_GET,
    [](
      AsyncWebServerRequest *request) {
      if (!sht45Data.valido) {

        request->send(
          503,
          "application/json",
          "{\"erro\":\"SHT45 sem leitura válida\"}");

        return;
      }

      StaticJsonDocument<384>
        jsonDoc;

      jsonDoc["temperatura_celsius"] =
        sht45Data.temperaturaCelsius;

      jsonDoc["temperatura_fahrenheit"] =
        sht45Data.temperaturaFahrenheit;

      jsonDoc["umidade"] =
        sht45Data.umidade;

      jsonDoc["data_hora"] =
        sht45Data.dataHora;

      jsonDoc["uptime"] =
        sht45Data.uptime;

      jsonDoc["sensor_ip"] =
        sht45Data.sensorIp;

      jsonDoc["rssi"] =
        sht45Data.rssi;

      AsyncResponseStream *
        response =
          request->beginResponseStream(
            "application/json");

      serializeJson(
        jsonDoc,
        *response);

      response->addHeader(
        "Access-Control-Allow-Origin",
        "*");

      response->addHeader(
        "Access-Control-Allow-Methods",
        "GET, POST, OPTIONS");

      response->addHeader(
        "Access-Control-Allow-Headers",
        "Content-Type, Authorization");

      request->send(response);
    });

  //----------------------------------
  // 404
  //----------------------------------

  server.onNotFound(
    [](
      AsyncWebServerRequest *request) {
      request->send(
        404,
        "text/plain",
        "❌ Pagina nao encontrada...");
    });

  //----------------------------------
  // Root
  //----------------------------------

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    handleRoot(request);
  });

  //----------------------------------
  // Info hardware
  //----------------------------------

  server.on(
    "/esp32/api/info",
    HTTP_GET,
    [](
      AsyncWebServerRequest *request) {
      handleJSON(request);
    });

  server.begin();
}

//------------------------------------------------
// LOOP
//------------------------------------------------

void loop() {

  esp_task_wdt_reset();

  //----------------------------------
  // Wi-Fi
  //----------------------------------

  if (WiFi.status() != WL_CONNECTED) {
    connectWiFi();
  }

  //----------------------------------
  // MQTT
  //----------------------------------

  if (!client.connected()) {
    reconnectMQTT();
  } else {
    client.loop();
  }

  //----------------------------------
  // LEITURA ÚNICA SHT45
  //----------------------------------

  if (
    millis() - lastSht45Read >= SHT45_READ_INTERVAL) {
    lastSht45Read = millis();

    if (updateSht45Data()) {

      //----------------------------------
      // WebSocket SHT45
      //----------------------------------

      if (wsSht45.count() > 0) {
        String json = generateSht45JSON();
        wsSht45.textAll(json);
      }
    }
  }

  //----------------------------------
  // WebSocket hardware
  //----------------------------------

  if (millis() - lastPrint > 2000) {
    lastPrint = millis();
    String json = generateJSON();
    ws.textAll(json);
  }

  //----------------------------------
  // MQTT
  //----------------------------------

  if (millis() - lastMqttPublish > 5000) {
    lastMqttPublish = millis();
    publishSensorData();
  }

  //----------------------------------
  // NTP
  //----------------------------------

  if (millis() - lastNtpSync > 3600000) {
    if (WiFi.status() == WL_CONNECTED) {
      lastNtpSync = millis();
      sntp_restart();
    }
  }
  delay(20);
}

//------------------------------------------------
// KB
//------------------------------------------------

float toKB(uint32_t bytes) {
  return bytes / 1024.0;
}

//------------------------------------------------
// Uptime formatado
//------------------------------------------------

String formatUptime(
  unsigned long seconds) {
  int d = seconds / 86400;
  int h = (seconds % 86400) / 3600;
  int m = (seconds % 3600) / 60;
  int s = seconds % 60;
  char buf[20];
  sprintf(buf, "%dd %02d:%02d:%02d", d, h, m, s);
  return String(buf);
}

//------------------------------------------------
// JSON monitor hardware
// NÃO ALTERADO
//------------------------------------------------

String generateJSON() {

  esp_chip_info_t chip_info;

  esp_chip_info(
    &chip_info);

  float heapTotal =
    toKB(
      ESP.getHeapSize());

  float heapFree =
    toKB(
      ESP.getFreeHeap());

  float heapMin =
    toKB(
      ESP.getMinFreeHeap());

  float heapUsedPercent =
    100.0 - ((heapFree / heapTotal) * 100.0);

  UBaseType_t stackWords =
    uxTaskGetStackHighWaterMark(
      NULL);

  uint32_t stackBytes =
    stackWords * 4;

  StaticJsonDocument<384>
    doc;

  uint64_t us =
    esp_timer_get_time();

  doc["uptime"] =
    formatUptime(
      us / 1000000);

  doc["cpu_mhz"] =
    getCpuFrequencyMhz();

  doc["sdk"] =
    ESP.getSdkVersion();

  doc["chip_cores"] =
    chip_info.cores;

  doc["chip_revision"] =
    chip_info.revision;

  doc["flash_kb"] =
    toKB(
      ESP.getFlashChipSize());

  doc["heap_total_kb"] =
    heapTotal;

  doc["heap_free_kb"] =
    heapFree;

  doc["heap_min_kb"] =
    heapMin;

  doc["heap_used_percent"] =
    heapUsedPercent;

  doc["stack_free_bytes"] =
    stackBytes;

  doc["wifi_ip"] =
    WiFi.localIP().toString();

  doc["wifi_rssi"] =
    WiFi.RSSI();

  String output;

  serializeJson(
    doc,
    output);

  return output;
}

//------------------------------------------------
// API info
//------------------------------------------------

void handleJSON(
  AsyncWebServerRequest *request) {

  request->send(
    200,
    "application/json",
    generateJSON());
}

//------------------------------------------------
// ROOT
// Seu HTML original permanece aqui.
//------------------------------------------------

void handleRoot(
  AsyncWebServerRequest *request) {

  String html = R"rawliteral( 
<!DOCTYPE html> 
<html> 
<head> 
<meta charset="UTF-8"> 
<title>ESP32 System Monitor - Real-Time Embedded Telemetry Dashboard</title> 
<script src="https://cdn.jsdelivr.net/npm/chart.js"></script>

<style>
*{box-sizing:border-box;margin:0;padding:0}

body{
  font-family:Segoe UI,sans-serif;
  background:linear-gradient(135deg,#0f172a,#1e293b,#0f172a);
  color:white;
  padding:25px;
}

.title{text-align:center;margin-bottom:5px;}
.subtitle{text-align:center;color:#94a3b8;margin-bottom:10px;font-size:14px;letter-spacing:1px;}
.subtitle-emp{text-align:center;color:#94a3b8;margin-top:-10px;margin-bottom:30px;font-size:10px;letter-spacing:1px;}

.title h1{
  font-size:42px;
  font-weight:900;
  letter-spacing:1px;
  background:linear-gradient(90deg,#22c55e,#38bdf8,#a855f7,#f43f5e,#22c55e);
  background-size:300% 300%;
  -webkit-background-clip:text;
  -webkit-text-fill-color:transparent;
  animation:gradientMove 6s ease infinite;
  text-shadow:0 0 25px rgba(56,189,248,0.35);
}

@keyframes gradientMove{
  0%{background-position:0% 50%;}
  50%{background-position:100% 50%;}
  100%{background-position:0% 50%;}
}

.dashboard{
  display:grid;
  grid-template-columns:repeat(auto-fit,minmax(180px,1fr));
  gap:14px;
  margin-bottom:30px;
}

.card{
  background:rgba(255,255,255,0.06);
  backdrop-filter:blur(12px);
  border-radius:14px;
  padding:12px;
  text-align:center;
}

.label{font-size:12px;color:#94a3b8;}
.value{font-size:16px;margin-top:4px;font-weight:600;}

.progress{
  height:6px;
  background:rgba(255,255,255,0.1);
  border-radius:6px;
  margin-top:6px;
  overflow:hidden;
}

.progress-fill{
  height:100%;
  width:0%;
  background:linear-gradient(90deg,#22c55e,#38bdf8);
  transition:0.4s ease;
}

.rssi-status{
  margin-top:6px;
  display:flex;
  align-items:center;
  justify-content:center;
  gap:6px;
  font-size:13px;
}

.rssi-dot{
  width:10px;
  height:10px;
  border-radius:50%;
}

.charts{
  display:grid;
  grid-template-columns:1fr 1fr;
  gap:25px;
}

@media(max-width:768px){
  .charts{grid-template-columns:1fr;}
}

.chart-card{
  background:rgba(255,255,255,0.06);
  border-radius:18px;
  padding:20px;
  height:350px;
}

canvas{
  width:100%!important;
  height:100%!important;
}
</style>
</head>

<body>

<div class="title">
  <h1>ESP32 Hardware Monitor</h1>
</div>

<div class="subtitle">
  Painel de Telemetria Embarcada - Tempo Real
</div>

<div class="subtitle-emp">
  Captação de temperatura e umidade
</div>

<div class="dashboard">

  <div class="card">
    <div class="label">Uptime</div>
    <div class="value" id="uptime"></div>
  </div>

  <div class="card">
    <div class="label">CPU (MHz)</div>
    <div class="value" id="cpu"></div>
  </div>

  <div class="card">
    <div class="label">SDK</div>
    <div class="value" id="sdk"></div>
  </div>

  <div class="card">
    <div class="label">Cores</div>
    <div class="value" id="cores"></div>
  </div>

  <div class="card">
    <div class="label">Revision</div>
    <div class="value" id="revision"></div>
  </div>

  <div class="card">
    <div class="label">Flash (KB)</div>
    <div class="value" id="flash"></div>
  </div>

  <div class="card">

    <div class="label">
      Heap Used (%)
    </div>

    <div
      class="value"
      id="heapPercent"
    ></div>

    <div class="progress">
      <div
        class="progress-fill"
        id="heapBar"
      ></div>
    </div>

  </div>

  <div class="card">
    <div class="label">
      Heap Total (KB)
    </div>

    <div
      class="value"
      id="heapTotal"
    ></div>
  </div>

  <div class="card">
    <div class="label">
      Heap Free (KB)
    </div>

    <div
      class="value"
      id="heapFree"
    ></div>
  </div>

  <div class="card">
    <div class="label">
      Heap Min (KB)
    </div>

    <div
      class="value"
      id="heapMin"
    ></div>
  </div>

  <div class="card">

    <div class="label">
      WiFi RSSI
    </div>

    <div
      class="value"
      id="rssi"
    ></div>

    <div class="progress">
      <div
        class="progress-fill"
        id="rssiBar"
      ></div>
    </div>

    <div class="rssi-status">

      <div
        class="rssi-dot"
        id="rssiDot"
      ></div>

      <div id="rssiText"></div>

    </div>

  </div>

  <div class="card">
    <div class="label">IP</div>

    <div
      class="value"
      id="ip"
    ></div>
  </div>

</div>

<div class="charts">

  <div class="chart-card">
    <canvas id="heapChart"></canvas>
  </div>

  <div class="chart-card">
    <canvas id="wifiChart"></canvas>
  </div>

</div>

<script>

let heapData=[];
let wifiData=[];
let labels=[];

const heapChart=new Chart(
  document.getElementById('heapChart'),
  {
    type:'line',

    data:{
      labels:labels,

      datasets:[{
        label:'Heap Usage (%)',
        data:heapData,
        borderColor:'#22c55e',
        fill:false,
        tension:0.3,
        borderWidth:4,
        pointRadius:2,
        pointHoverRadius:6
      }]
    },

    options:{
      responsive:true,
      maintainAspectRatio:false
    }
  }
);

const wifiChart=new Chart(
  document.getElementById('wifiChart'),
  {
    type:'line',

    data:{
      labels:labels,

      datasets:[{
        label:'WiFi RSSI (dBm)',
        data:wifiData,
        borderColor:'#38bdf8',
        fill:false,
        tension:0.3,
        borderWidth:4,
        pointRadius:2,
        pointHoverRadius:6
      }]
    },

    options:{
      responsive:true,
      maintainAspectRatio:false
    }
  }
);

function getRssiStatus(rssi){

  if(rssi >= -50)
    return {
      color:"#22c55e",
      text:"Excelente"
    };

  if(rssi >= -67)
    return {
      color:"#22c55e",
      text:"Bom"
    };

  if(rssi >= -75)
    return {
      color:"#facc15",
      text:"Regular"
    };

  if(rssi >= -85)
    return {
      color:"#fb923c",
      text:"Fraco"
    };

  return {
    color:"#ef4444",
    text:"Muito fraco"
  };
}

const ws = new WebSocket(
  `ws://${location.host}/ws`
);

ws.onmessage = function(event){

  const j =
    JSON.parse(event.data);

  document.getElementById(
    "uptime"
  ).innerText=j.uptime;

  document.getElementById(
    "cpu"
  ).innerText=j.cpu_mhz;

  document.getElementById(
    "sdk"
  ).innerText=j.sdk;

  document.getElementById(
    "cores"
  ).innerText=j.chip_cores;

  document.getElementById(
    "revision"
  ).innerText=j.chip_revision;

  document.getElementById(
    "flash"
  ).innerText=j.flash_kb;

  document.getElementById(
    "heapTotal"
  ).innerText=j.heap_total_kb;

  document.getElementById(
    "heapFree"
  ).innerText=j.heap_free_kb;

  document.getElementById(
    "heapMin"
  ).innerText=j.heap_min_kb;

  document.getElementById(
    "heapPercent"
  ).innerText=
    j.heap_used_percent+"%";

  document.getElementById(
    "rssi"
  ).innerText=
    j.wifi_rssi+" dBm";

  document.getElementById(
    "ip"
  ).innerText=j.wifi_ip;

  document.getElementById(
    "heapBar"
  ).style.width=
    j.heap_used_percent+"%";

  let rssiPercent=
    ((j.wifi_rssi+100)/60)*100;

  rssiPercent=
    Math.max(
      0,
      Math.min(
        100,
        rssiPercent
      )
    );

  document.getElementById(
    "rssiBar"
  ).style.width=
    rssiPercent+"%";

  const status=
    getRssiStatus(
      j.wifi_rssi
    );

  document.getElementById(
    "rssiDot"
  ).style.background=
    status.color;

  document.getElementById(
    "rssiText"
  ).innerText=
    status.text;

  if(labels.length>30){

    labels.shift();
    heapData.shift();
    wifiData.shift();
  }

  labels.push("");

  heapData.push(
    j.heap_used_percent
  );

  wifiData.push(
    j.wifi_rssi
  );

  heapChart.update();
  wifiChart.update();
};

</script>

</body>
</html>
)rawliteral";

  request->send(
    200,
    "text/html",
    html);
}

//------------------------------------------------
// NTP
//------------------------------------------------

void syncTime() {

  Serial.println(
    "🕒 Iniciando sincronização NTP");

  setenv(
    "TZ",
    "BRT3",
    1);

  tzset();

  sntp_setoperatingmode(
    SNTP_OPMODE_POLL);

  sntp_setservername(
    0,
    "pool.ntp.org");

  sntp_setservername(
    1,
    "time.nist.gov");

  sntp_set_sync_mode(
    SNTP_SYNC_MODE_IMMED);

  sntp_set_sync_interval(
    3600000);

  sntp_init();

  struct tm timeinfo;

  int retry = 0;

  while (
    !getLocalTime(&timeinfo) && retry < 15) {

    Serial.print(".");

    delay(200);

    retry++;
  }

  if (retry < 15) {
    Serial.println("✅ Sincronização NTP executada");
    Serial.println();
  } else {
    Serial.println();
    Serial.println("❌ Falha ao sincronizar horário");
  }
}

//------------------------------------------------
// Wi-Fi
//------------------------------------------------

void connectWiFi() {

  if (
    WiFi.status() == WL_CONNECTED) {
    return;
  }

  WiFi.persistent(true);

  WiFi.setAutoReconnect(true);

  if (
    !WiFi.config(
      local_IP,
      gateway,
      subnet,
      primaryDNS,
      secondaryDNS)) {

    Serial.println(
      "❌ Falha ao configurar IP");
  }

  WiFi.mode(WIFI_STA);

  WiFi.setHostname(
    wifi_hostname);

  WiFi.begin(
    ssid,
    wifi_password);

  Serial.println(
    "⌛ Conectando-se à rede");

  unsigned long
    startAttemptTime =
      millis();

  while (
    WiFi.status() != WL_CONNECTED) {

    delay(500);

    if (
      millis() - startAttemptTime > 30000) {

      Serial.println(
        "❌ WiFi não conectou — "
        "Reiniciando ESP");

      ESP.restart();
    }
  }

  if (
    millis() - lastNtpSync > 3600000) {

    lastNtpSync =
      millis();

    sntp_restart();
  }

  Serial.println(
    "🌐 Conexão estabelecida IP: "
    + WiFi.localIP().toString());

  Serial.println(
    "🛜 Hostname: "
    + String(
      WiFi.getHostname()));
}

//------------------------------------------------
// MQTT reconnect
//------------------------------------------------

void reconnectMQTT() {

  static unsigned long
    lastAttempt = 0;

  if (
    millis() - lastAttempt < 10000) {
    return;
  }

  lastAttempt =
    millis();

  Serial.println("📡 Tentando conectar ao MQTT...");
  Serial.println();

  Serial.print("Broker: ");
  Serial.println(mqtt_server);

  Serial.print("Porta: ");
  Serial.println(8883);

  Serial.print("WiFi: ");
  Serial.println(WiFi.status());

  Serial.print("IP: ");
  Serial.println(WiFi.localIP());

  if (
    client.connect(
      client_id,
      mqtt_user,
      mqtt_password,
      "sensores/esp32/status",
      1,
      true,
      "offline")) {

    client.publish(
      "sensores/esp32/status",
      "online",
      true);

    Serial.println("✅ MQTT conectado: "+ String(client_id));
    Serial.println();

  } else {

    Serial.print(
      "❌ Conexão MQTT perdida rc=");

    Serial.println(
      client.state());

    Serial.print("MQTT state: ");
    Serial.println(client.state());

    Serial.print("MQTT error string: ");

    switch (client.state()) {

      case -4:
        Serial.println("MQTT_CONNECTION_TIMEOUT");
        break;

      case -3:
        Serial.println("MQTT_CONNECTION_LOST");
        break;

      case -2:
        Serial.println("MQTT_CONNECT_FAILED");
        break;

      case -1:
        Serial.println("MQTT_DISCONNECTED");
        break;

      case 0:
        Serial.println("MQTT_CONNECTED");
        break;

      case 1:
        Serial.println("MQTT_CONNECT_BAD_PROTOCOL");
        break;

      case 2:
        Serial.println("MQTT_CONNECT_BAD_CLIENT_ID");
        break;

      case 3:
        Serial.println("MQTT_CONNECT_UNAVAILABLE");
        break;

      case 4:
        Serial.println("MQTT_CONNECT_BAD_CREDENTIALS");
        break;

      case 5:
        Serial.println("MQTT_CONNECT_UNAUTHORIZED");
        break;

      default:
        Serial.println("UNKNOWN");
        break;
    }
  }
}

//------------------------------------------------
// MQTT publish
//------------------------------------------------

void publishSensorData() {

  if (!client.connected()) {

    Serial.println(
      "⚠ MQTT desconectado. "
      "Publish ignorado.");

    return;
  }

  if (!sht45Data.valido) {
    Serial.println("⚠ SHT45 sem leitura válida. Publish ignorado.");
    return;
  }

  StaticJsonDocument<384>
    jsonDoc;

  jsonDoc["temperatura_celsius"] = sht45Data.temperaturaCelsius;
  jsonDoc["temperatura_fahrenheit"] = sht45Data.temperaturaFahrenheit;
  jsonDoc["umidade"] = sht45Data.umidade;
  jsonDoc["data_hora"] = sht45Data.dataHora;
  jsonDoc["uptime"] = sht45Data.uptime;
  jsonDoc["sensor_ip"] = sht45Data.sensorIp;
  jsonDoc["rssi"] = sht45Data.rssi;

  String jsonString;

  serializeJson(
    jsonDoc,
    jsonString);

  bool publicado = client.publish(mqtt_topic, jsonString.c_str());

  if (publicado) {
    Serial.print("📤 MQTT SENT: ");
    Serial.println(jsonString);

  } else {
    Serial.println("❌ Falha ao publicar MQTT");
  }
}
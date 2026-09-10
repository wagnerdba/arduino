/*******************************************************************
 * XIAO ESP32-C5 + BLYNK
 * Arduino ESP32 Core 3.3.11
 *
 * XIAO ESP32-C5 - Seeed Studio
 *
 * V18  -> comando do Blynk
 * GPIO2 / D1 -> saída digital física
 * V2   -> uptime DDD:HH:MM:SS
 *
 * by Wagner Pires
 *******************************************************************/

#define BLYNK_TEMPLATE_ID "TMPL2_qz8SVMB"
#define BLYNK_TEMPLATE_NAME "CASA FAMÍLIA PIRES"
#define BLYNK_AUTH_TOKEN "D32bxajk_8ofNvlCcLjdHe7yxUhZdlOn"

#define BLYNK_PRINT Serial

#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>

#include <esp_system.h>
#include <esp_task_wdt.h>


// ================================================================
// WIFI
// ================================================================

char ssid[] = "GABRIEL_HOME_5G";
char pass[] = "@FlakE2021#";


// ================================================================
// GPIO DO XIAO ESP32-C5
// ================================================================
//
// D1 = GPIO2
//
// V18 do Blynk NÃO é GPIO18.
// V18 é somente o canal virtual do Blynk.
//
#define DIGITAL_PIN 2


// ================================================================
// BLYNK
// ================================================================

BlynkTimer timer;


// ================================================================
// UPTIME
// ================================================================

unsigned long previousMillis = 0;
unsigned long elapsedMillis = 0;


// ================================================================
// CONTROLE DE CONEXÃO
// ================================================================

unsigned long ultimoWiFiCheck = 0;
unsigned long ultimoBlynkCheck = 0;

const unsigned long INTERVALO_WIFI_CHECK = 5000;
const unsigned long INTERVALO_BLYNK_CHECK = 5000;


// ================================================================
// WATCHDOG
// ================================================================

const uint32_t WDT_TIMEOUT_MS = 8000;


// ================================================================
// V18 - COMANDO DIGITAL
// ================================================================

BLYNK_WRITE(V18) {
  int value = param.asInt();

  Serial.print("Blynk V18 = ");
  Serial.println(value);

  if (value) {
    digitalWrite(DIGITAL_PIN, HIGH);
    Serial.println("GPIO2 -> HIGH");
  } else {
    digitalWrite(DIGITAL_PIN, LOW);
    Serial.println("GPIO2 -> LOW");
  }

  // Mantém o estado no Blynk
  Blynk.virtualWrite(V18, value);
}


// ================================================================
// BLYNK CONNECTED
// ================================================================

BLYNK_CONNECTED() {
  Serial.println();
  Serial.println("================================");
  Serial.println("Conectado à Cloud Blynk");
  Serial.println("================================");

  // Recupera o último estado do V18
  Blynk.syncVirtual(V18);
}


// ================================================================
// UPTIME
// ================================================================

void myTimerEvent() {
  unsigned long currentMillis = millis();

  elapsedMillis += currentMillis - previousMillis;
  previousMillis = currentMillis;


  unsigned long dias =
    elapsedMillis / 86400000UL;

  unsigned long horas =
    (elapsedMillis % 86400000UL) / 3600000UL;

  unsigned long minutos =
    (elapsedMillis % 3600000UL) / 60000UL;

  unsigned long segundos =
    (elapsedMillis % 60000UL) / 1000UL;


  char texto[20];

  sprintf(
    texto,
    "%03lu:%02lu:%02lu:%02lu",
    dias,
    horas,
    minutos,
    segundos);


  // Envia uptime para V2
  if (Blynk.connected()) {
    Blynk.virtualWrite(V2, texto);
  }
}


// ================================================================
// CONECTAR WIFI
// ================================================================

bool conectaWiFi() {
  if (WiFi.status() == WL_CONNECTED) {
    return true;
  }


  Serial.println();
  Serial.println("================================");
  Serial.println("Conectando ao WiFi...");
  Serial.println("================================");


  WiFi.mode(WIFI_STA);

  WiFi.begin(ssid, pass);


  unsigned long inicio = millis();


  while (
    WiFi.status() != WL_CONNECTED && millis() - inicio < 15000) {
    delay(500);

    Serial.print(".");

    // Alimenta watchdog durante a espera
    esp_task_wdt_reset();
  }


  Serial.println();


  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("WiFi conectado!");

    Serial.print("SSID: ");
    Serial.println(WiFi.SSID());

    Serial.print("IP: ");
    Serial.println(WiFi.localIP());

    Serial.print("RSSI: ");
    Serial.print(WiFi.RSSI());
    Serial.println(" dBm");

    return true;
  }


  Serial.println("WiFi não conectado.");

  return false;
}


// ================================================================
// CONECTAR BLYNK
// ================================================================

bool conectaBlynk() {
  if (WiFi.status() != WL_CONNECTED) {
    return false;
  }


  if (Blynk.connected()) {
    return true;
  }


  Serial.println();
  Serial.println("Conectando ao Blynk...");


  Blynk.config(BLYNK_AUTH_TOKEN);


  unsigned long inicio = millis();


  while (
    !Blynk.connected() && millis() - inicio < 10000) {
    Blynk.connect(1000);

    esp_task_wdt_reset();

    delay(10);
  }


  if (Blynk.connected()) {
    Serial.println("Blynk conectado!");

    return true;
  }


  Serial.println("Blynk não conectado.");

  return false;
}


// ================================================================
// MOTIVO DO ÚLTIMO RESET
// ================================================================

void mostraMotivoReset() {
  esp_reset_reason_t resetReason =
    esp_reset_reason();


  Serial.println();
  Serial.println("================================");
  Serial.println("MOTIVO DO ÚLTIMO RESET");
  Serial.println("================================");


  Serial.print("Código: ");
  Serial.println((int)resetReason);


  switch (resetReason) {
    case ESP_RST_POWERON:
      Serial.println("Power ON");
      break;


    case ESP_RST_EXT:
      Serial.println("Reset externo");
      break;


    case ESP_RST_SW:
      Serial.println("Software reset");
      break;


    case ESP_RST_PANIC:
      Serial.println("PANIC");
      break;


    case ESP_RST_INT_WDT:
      Serial.println("Interrupt Watchdog");
      break;


    case ESP_RST_TASK_WDT:
      Serial.println("Task Watchdog");
      break;


    case ESP_RST_WDT:
      Serial.println("Outro Watchdog");
      break;


    case ESP_RST_DEEPSLEEP:
      Serial.println("Deep Sleep");
      break;


    case ESP_RST_BROWNOUT:
      Serial.println("Brownout");
      break;


    case ESP_RST_SDIO:
      Serial.println("SDIO");
      break;


    default:
      Serial.println("Desconhecido");
      break;
  }


  Serial.println("================================");
}


// ================================================================
// SETUP
// ================================================================

void setup() {
  Serial.begin(115200);

  delay(1000);


  Serial.println();
  Serial.println();
  Serial.println("================================");
  Serial.println("XIAO ESP32-C5 + BLYNK");
  Serial.println("Arduino Core 3.3.11");
  Serial.println("================================");


  Serial.print("Chip: ");
  Serial.println(ESP.getChipModel());


  Serial.print("CPU: ");
  Serial.print(ESP.getCpuFreqMHz());
  Serial.println(" MHz");


  Serial.print("Flash: ");
  Serial.print(
    ESP.getFlashChipSize() / 1024 / 1024);
  Serial.println(" MB");


  Serial.print("PSRAM: ");
  Serial.print(
    ESP.getPsramSize() / 1024 / 1024);
  Serial.println(" MB");


  mostraMotivoReset();


  // ============================================================
  // GPIO
  // ============================================================

  pinMode(DIGITAL_PIN, OUTPUT);

  digitalWrite(DIGITAL_PIN, LOW);


  Serial.println();
  Serial.println("GPIO2 / D1 configurado como OUTPUT");
  Serial.println("Estado inicial: LOW");


  // ============================================================
  // WATCHDOG
  // ============================================================

  Serial.println();
  Serial.println("Configurando watchdog...");

  esp_task_wdt_add(NULL);

  Serial.println("Watchdog já inicializado pelo sistema.");
  Serial.println("Tarefa atual adicionada ao watchdog.");


  Serial.println("Watchdog configurado: 8 segundos");


  // ============================================================
  // WIFI
  // ============================================================

  conectaWiFi();


  // ============================================================
  // BLYNK
  // ============================================================

  if (WiFi.status() == WL_CONNECTED) {
    conectaBlynk();
  }


  // ============================================================
  // TIMER
  // ============================================================

  previousMillis = millis();

  timer.setInterval(
    1000L,
    myTimerEvent);


  Serial.println();
  Serial.println("================================");
  Serial.println("SETUP FINALIZADO");
  Serial.println("================================");
}


// ================================================================
// LOOP
// ================================================================

void loop() {
  // ------------------------------------------------------------
  // Alimenta watchdog
  // ------------------------------------------------------------

  esp_task_wdt_reset();


  // ------------------------------------------------------------
  // Verifica WiFi
  // ------------------------------------------------------------

  if (
    millis() - ultimoWiFiCheck >= INTERVALO_WIFI_CHECK) {
    ultimoWiFiCheck = millis();


    if (WiFi.status() != WL_CONNECTED) {
      Serial.println();
      Serial.println("WiFi desconectado!");

      conectaWiFi();
    }
  }


  // ------------------------------------------------------------
  // Verifica Blynk
  // ------------------------------------------------------------

  if (
    millis() - ultimoBlynkCheck >= INTERVALO_BLYNK_CHECK) {
    ultimoBlynkCheck = millis();


    if (
      WiFi.status() == WL_CONNECTED && !Blynk.connected()) {
      Serial.println();
      Serial.println("Blynk desconectado!");

      conectaBlynk();
    }
  }


  // ------------------------------------------------------------
  // BLYNK
  // ------------------------------------------------------------

  if (Blynk.connected()) {
    Blynk.run();
  }


  // ------------------------------------------------------------
  // TIMER
  // ------------------------------------------------------------

  timer.run();


  // ------------------------------------------------------------
  // Pequeno descanso
  // ------------------------------------------------------------

  delay(5);
}
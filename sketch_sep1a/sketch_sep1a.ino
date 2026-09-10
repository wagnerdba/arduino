#include <WiFi.h>

const char* ssid = "GABRIEL_HOME_5G";
const char* pass = "@FlakE2021#";

void setup() {
  Serial.begin(115200);

  delay(1000);

  Serial.println();
  Serial.println("==============================");
  Serial.println("XIAO ESP32-C5 TESTE");
  Serial.println("==============================");

  Serial.print("Chip: ");
  Serial.println(ESP.getChipModel());

  Serial.print("Flash: ");
  Serial.print(ESP.getFlashChipSize() / 1024 / 1024);
  Serial.println(" MB");

  Serial.print("PSRAM: ");
  Serial.print(ESP.getPsramSize() / 1024 / 1024);
  Serial.println(" MB");

  Serial.println();
  Serial.println("Conectando ao WiFi...");

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, pass);

  int tentativas = 0;

  while (WiFi.status() != WL_CONNECTED && tentativas < 30) {
    delay(500);
    Serial.print(".");
    tentativas++;
  }

  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("WiFi conectado!");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("WiFi NAO conectado.");
  }
}

void loop() {
  delay(1000);

  Serial.print("WiFi status: ");
  Serial.println(WiFi.status());
}
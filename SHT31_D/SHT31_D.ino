#include <Wire.h>
#include <Adafruit_SHT31.h>

#define SDA_PIN 21
#define SCL_PIN 22

Adafruit_SHT31 sht31 = Adafruit_SHT31();

void setup() {
  Serial.begin(115200);
  Wire.begin(SDA_PIN, SCL_PIN);

  if (!sht31.begin(0x44)) { // tente 0x45 se não achar
    Serial.println("❌ SHT31 não encontrado");
    while (1);
  }

  Serial.println("✅ SHT31 iniciado");
}

void loop() {
  float temp = sht31.readTemperature();
  float hum  = sht31.readHumidity();

  if (!isnan(temp) && !isnan(hum)) {

    float tempF = temp * 1.8 + 32.0;

    Serial.print("Temperature: ");
    Serial.print(temp);
    Serial.print("°C | ");
    Serial.print(tempF);
    Serial.print("°F | Umidity: ");
    Serial.print(hum);
    Serial.println("%");
  } else {
    Serial.println("⚠️ Erro de leitura SHT31");
  }

  delay(10000);
}

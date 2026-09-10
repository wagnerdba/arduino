#include <Wire.h>
#include <Adafruit_SHT4x.h>

#define SDA_PIN 22
#define SCL_PIN 21

Adafruit_SHT4x sht45;

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println();
  Serial.println("=== TESTE SHT45 ===");

  Wire.begin(SDA_PIN, SCL_PIN);
  Wire.setClock(100000);

  delay(100);

  Serial.println("Scanner I2C:");

  bool encontrado = false;

  for (uint8_t addr = 1; addr < 127; addr++) {
    Wire.beginTransmission(addr);

    if (Wire.endTransmission() == 0) {
      Serial.printf("  -> Encontrado: 0x%02X\n", addr);
      encontrado = true;
    }
  }

  if (!encontrado) {
    Serial.println("  -> NENHUM dispositivo encontrado");
  }

  Serial.println();

  if (!sht45.begin(&Wire)) {
    Serial.println("❌ SHT45 não encontrado");
    return;
  }

  Serial.println("✅ SHT45 encontrado!");

  sht45.setPrecision(SHT4X_HIGH_PRECISION);
  sht45.setHeater(SHT4X_NO_HEATER);
}

void loop() {
  sensors_event_t humidity;
  sensors_event_t temperature;

  if (sht45.getEvent(&humidity, &temperature)) {

    Serial.print("Temperatura: ");
    Serial.print(temperature.temperature, 2);
    Serial.println(" °C");

    Serial.print("Umidade: ");
    Serial.print(humidity.relative_humidity, 2);
    Serial.println(" %");

  } else {
    Serial.println("❌ Erro na leitura");
  }

  delay(2000);
}
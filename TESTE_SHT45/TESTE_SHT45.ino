#include <Wire.h>
#include <Adafruit_SHT4x.h>

#define SDA_PIN 23
#define SCL_PIN 19

Adafruit_SHT4x sht4;

void scanI2C() {

  byte encontrados = 0;

  Serial.println();
  Serial.println("=== Scanner I2C ===");

  for (byte endereco = 1; endereco < 127; endereco++) {

    Wire.beginTransmission(endereco);

    if (Wire.endTransmission() == 0) {

      Serial.print("Dispositivo encontrado em 0x");

      if (endereco < 16)
        Serial.print("0");

      Serial.println(endereco, HEX);

      encontrados++;
    }

    delay(2);
  }

  if (encontrados == 0)
    Serial.println("Nenhum dispositivo encontrado");

  Serial.println("===================");
  Serial.println();
}

void setup() {

  Serial.begin(115200);

  delay(2000);

  Serial.println();
  Serial.println("Teste SHT45");

  Wire.begin(SDA_PIN, SCL_PIN);

  scanI2C();

  if (!sht4.begin()) {

    Serial.println("❌ SHT45 nao encontrado");

    return;
  }

  sht4.setPrecision(SHT4X_HIGH_PRECISION);
  sht4.setHeater(SHT4X_NO_HEATER);

  Serial.println("✅ SHT45 encontrado");
}

void loop() {

  sensors_event_t humidity;
  sensors_event_t temperature;

  sht4.getEvent(&humidity, &temperature);

  if (isnan(temperature.temperature) ||
      isnan(humidity.relative_humidity)) {

    Serial.println("❌ Erro de leitura");

  } else {

    Serial.print("Temperatura: ");
    Serial.print(temperature.temperature);
    Serial.print(" °C");

    Serial.print(" | Umidade: ");
    Serial.print(humidity.relative_humidity);
    Serial.println(" %");
  }

  delay(2000);
}

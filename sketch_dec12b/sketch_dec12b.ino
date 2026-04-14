void setup() {
  Serial.begin(115200);   // Inicia a serial

}

void loop() {
  Serial.println("Olá mundo");
  delay(3000);            // Aguarda estabilizar
}
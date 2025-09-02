#include <heltec_unofficial.h>

int counter = 0;           // O contador começa em 0
bool showProgressBar = true; // Controla se a barra de progresso será exibida

void setup() {
  heltec_setup();
  Serial.println();
  display.init();
  display.setFont(ArialMT_Plain_16);
}

void drawProgressBar() {
  int progress = (counter / 5) % 101; // Progresso de 0 a 100

  // Desenha a barra de progresso
  display.drawProgressBar(0, 32, 120, 10, progress);

  // Mostra a porcentagem
  display.setTextAlignment(TEXT_ALIGN_CENTER);
  display.drawString(64, 15, String(progress) + "%");

  // Quando o progresso atinge 100%, desativa a barra e limpa a tela
  if (progress >= 100) {
    showProgressBar = false;
    display.display();
    delay(500); // Pausa para mostrar a barra completa
    display.clear(); // Limpa a tela
    counter = 0; // Reinicia o contador para começar após a barra
  }
}

void drawCounter() {
  // Mostra o contador
  int loopCounter = counter % 10001; // Loop de 0 a 10000

  display.setFont(ArialMT_Plain_10);
  display.drawString(64, 0, "EMISSOR LoRa 32");
//  display.setFont(ArialMT_Plain_16);

  display.setTextAlignment(TEXT_ALIGN_CENTER);
  display.drawString(64, 23, "Enviando pacote: " + String(loopCounter));

  display.setFont(ArialMT_Plain_10);
  display.drawString(64, 50, "by Francisco Gamarra");
  display.setFont(ArialMT_Plain_16);


}

void loop() {
  heltec_loop();
  display.clear();

  if (showProgressBar) {
    drawProgressBar();
    counter++; // Incrementa o contador para a barra de progresso funcionar
  } else {
    drawCounter();
    counter++; // O contador só incrementa após a barra de progresso
  }

  display.display();
  delay(10);
}

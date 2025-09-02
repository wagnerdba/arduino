// Métrica	                          O que mede?	                                Valores bons         	Valores ruins
// RSSI (Intensidade do sinal)	      Força do sinal recebido	                    -30 a -90 dBm        	Abaixo de -100 dBm
// SNR (Relação sinal-ruído)	        Qualidade do sinal comparado ao ruído	      Acima de 8 dB	        Abaixo de 0 dB

#define HELTEC_POWER_BUTTON
#include <heltec_unofficial.h>

#define PAUSE               1    // Envia pacotes a cada 1 segundos
#define FREQUENCY           868.0  // Frequência alterada para 915 MHz
#define BANDWIDTH           62.5

// SF	     Alcance	        Velocidade	  Consumo de Energia  	Tempo de Transmissão
// SF7      Baixo (~1 km)     Rápido 🚀  	Baixo ⚡                 	Curto ⏳
// SF9	    Médio (~5 km)     Médio        Médio	                    Médio
// SF12	    Alto (~10+ km)    Lento 🐢	  Alto 🔋	                   Longo 🕒
#define SPREADING_FACTOR    7 // 12

#define TRANSMIT_POWER      22

String rxdata;
volatile bool rxFlag = false;
long counter = 0;
uint64_t last_tx = 0;
uint64_t tx_time;
//uint64_t minimum_pause;

void setup() {
  heltec_setup();

  // display.init(); inverte o display

  //both.println("Radio init");
  RADIOLIB_OR_HALT(radio.begin());
  radio.setDio1Action(rx);
  both.printf("Frequency: %.2f MHz\n", FREQUENCY);
  RADIOLIB_OR_HALT(radio.setFrequency(FREQUENCY));
  both.printf("Bandwidth: %.1f kHz\n", BANDWIDTH);
  RADIOLIB_OR_HALT(radio.setBandwidth(BANDWIDTH));
  both.printf("Spreading Factor: %i\n", SPREADING_FACTOR);
  RADIOLIB_OR_HALT(radio.setSpreadingFactor(SPREADING_FACTOR));
  both.printf("TX power: %i dBm\n", TRANSMIT_POWER);
  RADIOLIB_OR_HALT(radio.setOutputPower(TRANSMIT_POWER));
  RADIOLIB_OR_HALT(radio.startReceive(RADIOLIB_SX126X_RX_TIMEOUT_INF));
}

void loop() {
  heltec_loop();
  
  // Verifica se já passou 1 segundo desde o último envio
  if (millis() - last_tx >= PAUSE * 1000) { 
    both.printf("TX [%s] SEND ", String(counter).c_str());
    radio.clearDio1Action();
    heltec_led(10);
    tx_time = millis();
    RADIOLIB(radio.transmit(String(counter++).c_str()));
    tx_time = millis() - tx_time;
    heltec_led(0);
    if (_radiolib_status == RADIOLIB_ERR_NONE) {
      both.printf("OK (%i ms)\n", (int)tx_time);
    } else {
      both.printf("fail (%i)\n", _radiolib_status);
    }
    last_tx = millis(); // Atualiza o tempo do último envio
    radio.setDio1Action(rx);
    RADIOLIB_OR_HALT(radio.startReceive(RADIOLIB_SX126X_RX_TIMEOUT_INF));
  }
}

void rx() {
  rxFlag = true;
}

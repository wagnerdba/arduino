#define HELTEC_POWER_BUTTON
#include <heltec_unofficial.h>

#define FREQUENCY           868.0
#define BANDWIDTH           62.5
#define SPREADING_FACTOR    7 //12
#define TRANSMIT_POWER      22

String rxdata;
volatile bool rxFlag = false;

void setup() {
  heltec_setup();
  
   // display.init(); // inverte o display

   display.setFont(ArialMT_Plain_16);
   display.setTextAlignment(TEXT_ALIGN_CENTER);
   display.write("Receptor\n");
   display.write("Buscando Sinal...\n");
 //  display.setFont(ArialMT_Plain_10);

 // both.println("Radio init");
  RADIOLIB_OR_HALT(radio.begin());
  radio.setDio1Action(rx);
 // both.printf("Frequency: %.2f MHz\n", FREQUENCY);
  RADIOLIB_OR_HALT(radio.setFrequency(FREQUENCY));
 // both.printf("Bandwidth: %.1f kHz\n", BANDWIDTH);
  RADIOLIB_OR_HALT(radio.setBandwidth(BANDWIDTH));

 // both.printf("Spreading Factor: %i\n", SPREADING_FACTOR);
  RADIOLIB_OR_HALT(radio.setSpreadingFactor(SPREADING_FACTOR));
 // both.printf("TX power: %i dBm\n", TRANSMIT_POWER);
  
  RADIOLIB_OR_HALT(radio.setOutputPower(TRANSMIT_POWER));
 
  RADIOLIB_OR_HALT(radio.startReceive(RADIOLIB_SX126X_RX_TIMEOUT_INF));

  display.setFont(ArialMT_Plain_10);

}

void loop() {
  heltec_loop();

  if (rxFlag) {
    rxFlag = false;
    radio.readData(rxdata);
    if (_radiolib_status == RADIOLIB_ERR_NONE) {

      // display.drawString(64, 50, "by Francisco Gamarra");

      // display.drawString(64, 23, "RX [" + String(rxdata.c_str()) + "]");

      // both.printf("RECEPTOR LoRa %s", "32\n");

      heltec_led(10);

      both.printf("  RX [%s] RECEIVED\n", rxdata.c_str());
      both.printf("  RSSI: %.2f dBm\n", radio.getRSSI());
      both.printf("  SNR: %.2f dB\n", radio.getSNR());

      heltec_led(0);
    }
    RADIOLIB_OR_HALT(radio.startReceive(RADIOLIB_SX126X_RX_TIMEOUT_INF));
  }
   display.clear();
}

void rx() {
  rxFlag = true;
}

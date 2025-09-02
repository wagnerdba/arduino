#include <heltec_unofficial.h>
#include "LoRaWan_APP.h"
//#include "Arduino.h"


int counter = 0;           // O contador começa em 0
bool showProgressBar = true; // Controla se a barra de progresso será exibida

#define RF_FREQUENCY                                915000000 // Hz

#define TX_OUTPUT_POWER                             20        // dBm // 5

#define LORA_BANDWIDTH                              0         // [0: 125 kHz,
                                                              //  1: 250 kHz,
                                                              //  2: 500 kHz,
                                                              //  3: Reserved]
#define LORA_SPREADING_FACTOR                       12        // [SF7..SF12] // 7
#define LORA_CODINGRATE                             4         // [1: 4/5, // 1
                                                              //  2: 4/6,
                                                              //  3: 4/7,
                                                              //  4: 4/8]
#define LORA_PREAMBLE_LENGTH                        8         // Same for Tx and Rx
#define LORA_SYMBOL_TIMEOUT                         0         // Symbols
#define LORA_FIX_LENGTH_PAYLOAD_ON                  false
#define LORA_IQ_INVERSION_ON                        false


#define RX_TIMEOUT_VALUE                            1000
#define BUFFER_SIZE                                 30 // Define the payload size here

char txpacket[BUFFER_SIZE];
char rxpacket[BUFFER_SIZE];

double txNumber;

bool lora_idle=true;

static RadioEvents_t RadioEvents;
void OnTxDone( void );
void OnTxTimeout( void );


void setup() {
   Serial.begin(115200);
   Mcu.begin(HELTEC_BOARD,SLOW_CLK_TPYE);
	
    txNumber=0;

    RadioEvents.TxDone = OnTxDone;
    RadioEvents.TxTimeout = OnTxTimeout;
    
    Radio.Init( &RadioEvents );
    Radio.SetChannel( RF_FREQUENCY );
    Radio.SetTxConfig( MODEM_LORA, TX_OUTPUT_POWER, 0, LORA_BANDWIDTH,
                                   LORA_SPREADING_FACTOR, LORA_CODINGRATE,
                                   LORA_PREAMBLE_LENGTH, LORA_FIX_LENGTH_PAYLOAD_ON,
                                   true, 0, 0, LORA_IQ_INVERSION_ON, 3000 ); 

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

    /*
          if(lora_idle == true)
          {
            delay(2000);
            txNumber += 0.01;

            display.setTextAlignment(TEXT_ALIGN_CENTER);
            display.drawString(64, 23, "Enviando pacote: " + String(txNumber));

    //        Serial.printf("\r\nsending packet \"%s\" , length %d\r\n",txpacket, strlen(txpacket));

            Radio.Send( (uint8_t *)txpacket, strlen(txpacket) ); //send the package out	
            lora_idle = false;
          }
          Radio.IrqProcess( );
    */


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

void OnTxDone( void )
{
	Serial.println("TX done (mensagem enviada)...");
	lora_idle = true;
}

void OnTxTimeout( void )
{
    Radio.Sleep( );
    Serial.println("TX Timeout......");
    lora_idle = true;
}

const int pinSolo = 34;
const int pinRele = 27;

const int LIMITE_SECO = 3000;
const int LIMITE_MOLHADO = 2000;

const unsigned long TEMPO_REGA = 5000;
const unsigned long TEMPO_ESPERA = 30000;

bool regando = false;
unsigned long inicioRega = 0;
unsigned long fimRega = 0;

int lerSolo() {
  long soma = 0;

  for (int i = 0; i < 20; i++) {
    soma += analogRead(pinSolo);
    delay(10);
  }

  return soma / 20;
}

void ligarRega() {
  digitalWrite(pinRele, HIGH);
  regando = true;
  inicioRega = millis();

  Serial.println(">>> REGA LIGADA");
}

void desligarRega() {
  digitalWrite(pinRele, LOW);
  regando = false;
  fimRega = millis();

  Serial.println(">>> REGA DESLIGADA");
}

void setup() {
  Serial.begin(115200);

  pinMode(pinSolo, INPUT);
  pinMode(pinRele, OUTPUT);

  // Relé desligado ao iniciar
  digitalWrite(pinRele, LOW);

  Serial.println("Regador automatico iniciado");
}

void loop() {

  static unsigned long numeroLeitura = 0;

  numeroLeitura++;

  if (numeroLeitura > 20) {
    numeroLeitura = 1;
  }

  int valor = lerSolo();

  Serial.print(numeroLeitura);
  Serial.print(" - Umidade ADC: ");
  Serial.print(valor);

  if (regando) {
    Serial.println(" | REGANDO");

    // Tempo máximo da rega
    if (millis() - inicioRega >= TEMPO_REGA) {
      desligarRega();
    }

    delay(1000);
    return;
  }

  // Aguarda um tempo depois da última rega
  if (millis() - fimRega < TEMPO_ESPERA) {
    Serial.println(" | aguardando");
    delay(1000);
    return;
  }

  // Solo seco -> regar
  if (valor > LIMITE_SECO) {
    Serial.println(" | SOLO SECO");
    ligarRega();
  }
  // Solo suficientemente molhado -> não faz nada
  else if (valor < LIMITE_MOLHADO) {
    Serial.println(" | SOLO MOLHADO");
  } else {
    Serial.println(" | SOLO INTERMEDIARIO");
  }

  delay(1000);
}

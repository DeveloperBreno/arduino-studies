// arduino uno
// LED BUILTIN
void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.begin(9600); // Inicializa comunicação serial
}

void loop() {
  static unsigned long contador = 0;

  if (contador % 2 == 0) {
    // Par: pisca 2 vezes rápido
    for (int i = 0; i < 2; i++) {
      digitalWrite(LED_BUILTIN, HIGH);
      delay(100);
      digitalWrite(LED_BUILTIN, LOW);
      delay(100);
    }
  } else {
    // Ímpar: pisca 1 vez
    digitalWrite(LED_BUILTIN, HIGH);
    delay(200);
    digitalWrite(LED_BUILTIN, LOW);
    delay(200);
  }

  // Imprime número + par/ímpar
  if (contador % 2 == 0) {
    Serial.println(String(contador) + " - par");
  } else {
    Serial.println(String(contador) + " - ímpar");
  }

  contador++;
  delay(1000);
}

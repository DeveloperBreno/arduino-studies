#include <WiFi.h>
#include <BluetoothSerial.h>
#define LED_BUILTIN 4 // GPIO do LED embutido


void blinkMorseOK() {
  String morse = "--- -.-";  // OK em Morse
  for (int i = 0; i < morse.length(); i++) {
    char c = morse[i];
    if (c == '.') {
      digitalWrite(LED_BUILTIN, HIGH);
      delay(100);  // Ponto: curto
      digitalWrite(LED_BUILTIN, LOW);
    } else if (c == '-') {
      digitalWrite(LED_BUILTIN, HIGH);
      delay(400);  // Traço: longo
      digitalWrite(LED_BUILTIN, LOW);
    } else {
      delay(400);  // Espaço entre letras
    }
    delay(200);  // Espaço entre símbolos
  }
}

void blinkMorseSOS() {
  String morse = "... --- ...";  // SOS em código Morse
  for (int i = 0; i < morse.length(); i++) {
    char c = morse[i];
    if (c == '.') {
      digitalWrite(LED_BUILTIN, HIGH);
      delay(100);  // Ponto: curto
      digitalWrite(LED_BUILTIN, LOW);
    } else if (c == '-') {
      digitalWrite(LED_BUILTIN, HIGH);
      delay(400);  // Traço: longo
      digitalWrite(LED_BUILTIN, LOW);
    } else {
      delay(400);  // Espaço entre letras
    }
    delay(200);  // Espaço entre símbolos
  }
}



BluetoothSerial SerialBT;

String ssid = "";
String password = "";
bool wifiConfigured = false;



String sanitize(String text) {
  text.trim();                         // Remove espaços antes/depois
  text.replace("\r", "");              // Remove retorno de carro
  text.replace("\n", "");              // Remove quebra de linha
  return text;
}

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.begin(115200);
  SerialBT.begin("ESP32CAM_BT"); // Nome do dispositivo Bluetooth

  Serial.println("Aguardando dados via Bluetooth...");

  // teste
  // blinkMorseOK();
  // delay(1000);
  // blinkMorseSOS();

}

void loop() {
  if (SerialBT.available()) {
    String received = SerialBT.readStringUntil('\n');
    Serial.println("Recebido:");
    Serial.println(received);

    int sepIndex = received.indexOf(';');
    if (sepIndex != -1) {
      ssid = sanitize(received.substring(0, sepIndex));
      password = sanitize(received.substring(sepIndex + 1));
      wifiConfigured = true;
    }
  }

  if (wifiConfigured) {
    Serial.println("Tentando conectar no Wi-Fi:");
    Serial.println("SSID: " + ssid);
    Serial.println("Senha: " + password);

    WiFi.begin(ssid.c_str(), password.c_str());

    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 10) {
      delay(1000);
      Serial.print(".");
      attempts++;
    }

    if (WiFi.status() == WL_CONNECTED) {
      blinkMorseOK();  // Pisca "OK" quando conectado
      Serial.println("\n✅ Conectado com sucesso!");
      Serial.println("IP: " + WiFi.localIP().toString());
    } else {
      blinkMorseSOS();
      Serial.println("\n❌ Falha na conexão!");
    }

    wifiConfigured = false;
  }

  delay(100);
  
   if (WiFi.status() != WL_CONNECTED) {
    digitalWrite(LED_BUILTIN, HIGH);
    delay(400);  // Pisca para indicar erro
    digitalWrite(LED_BUILTIN, LOW);
    delay(5000);  // Intervalo entre piscadas
  }
}

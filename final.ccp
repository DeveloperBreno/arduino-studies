// bluetooth
#include <WiFi.h>
#include <BluetoothSerial.h>
#define LED_BUILTIN 4 // GPIO do LED embutido

// files
#include "SPIFFS.h"

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

// Caminho do arquivo
const char* path = "/c.txt";


String sanitize(String text) {
  text.trim();                         // Remove espaços antes/depois
  text.replace("\r", "");              // Remove retorno de carro
  text.replace("\n", "");              // Remove quebra de linha
  return text;
}

void setup() {
  // serial monitor config
  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);
  SerialBT.begin("ESP32CAM_BT"); // Nome do dispositivo Bluetooth

  
  if (!SPIFFS.begin(true)) {
    Serial.println("Erro ao montar SPIFFS");
    return;
  }

  Serial.println("SPIFFS montado com sucesso");

  if (!SPIFFS.exists(path)) {
   
    // Cria o arquivo (escreve nada nele)
    File file = SPIFFS.open(path, FILE_WRITE);
    if (!file) {
      Serial.println("Erro ao criar o arquivo");
      return;
    }

    // Fecha sem escrever nada
    file.close();
    Serial.println(); 
    Serial.println("Arquivo criado (conteudo): " + file.readString());
    Serial.println("Aguardando dados via Bluetooth...");

  }else{

    Serial.println("⚠️ Arquivo já configurado, conteudo");
    File fr = SPIFFS.open(path);
    String content = fr.readString();
    fr.close();
    Serial.println(content);

    int sepIndex = content.indexOf(';');
    Serial.println(content);
    Serial.println(sepIndex);
    if (sepIndex != -1) {
      ssid = content.substring(0, sepIndex);
      password = content.substring(sepIndex + 1);
      ssid.trim();
      password.trim();
      Serial.println("SSID: " + ssid);
      Serial.println("Password: " + password);
      wifiConfigured = true;
    }

    
  }

  // teste
  // blinkMorseOK();
  // delay(1000);
  // blinkMorseSOS();

}

void loop() {



  // recebe por bluetooth
  if (SerialBT.available() && wifiConfigured == false) {
    String received = SerialBT.readStringUntil('\n');
    Serial.println("Recebido:");
    Serial.println(received);

    int sepIndex = received.indexOf(';');
    if (sepIndex != -1) {
      ssid = sanitize(received.substring(0, sepIndex));
      password = sanitize(received.substring(sepIndex + 1));
      wifiConfigured = true;

      File f = SPIFFS.open(path, FILE_WRITE);
      f.print(ssid + ";" + password);
      f.close();

      f = SPIFFS.open(path);
      Serial.println(f.readString());
      f.close();

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

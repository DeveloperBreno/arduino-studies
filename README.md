# Componentes que serão utilizados:
  Módulo ESP32 CAM WIFI BLUETOOTH
  CAMERA OV2640
  MODULO MB

# Fluxo de setup (setup de configuração, não será em todas as vezes que ligar o dispositivo, mas sempre que entender que não está conectado a um wifi e não existir um arquivo .json na memoria flash):
1) ligar 
2) verificar se exsite o arquivo .json, caso não tenha crie um
3) se conter conteudo no arquivo deve ler a senha e ssid do wifi e tentar se conctar
4) caso não entre no wifi, deve piscar o led (saida 4, led integrado, 225 = 100% ligado)  
5) o usuario utilizada o aplicativo serial bluetooh terminal, para enviar o nome do wifi e senha,  exemplo: "nome_do_fiwi;senha_do_wifi"
6) apos a placa receber a senha e nome, odispositivo deve salvar essas informações no arquivo .json, para quando ligar o dispositivo novamente tente se conctar automaticamente ao wifi (lembrando que o wifi será um ponto de acesso do celular)
7) apos se conectar ao wifi o dispositivo deve tirar uma foto a cada 15 segundos e dispara para um servidor

# exemplos de codigo em partes

## comunicação com o bluetooh

```
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
```

## exemplo de codigo para crud com arquivo:


```
#include "SPIFFS.h"

void setup() {
  Serial.begin(115200);

  // Inicializa o SPIFFS
  if (!SPIFFS.begin(true)) {
    Serial.println("Erro ao montar SPIFFS");
    return;
  }

  Serial.println("SPIFFS montado com sucesso");

  // Caminho do arquivo
  const char* path = "/teste.txt";

  // Cria o arquivo (escreve nada nele)
  File file = SPIFFS.open(path, FILE_WRITE);
  if (!file) {
    Serial.println("Erro ao criar o arquivo");
    return;
  }

  // Fecha sem escrever nada
  file.close();
  Serial.println("Arquivo criado (vazio): " + String(path));
}

void loop() {
  // Nada no loop
}
```
## exemplo de codigo para enviar ao servidor

```
#include "esp_camera.h"
#include <WiFi.h>
#include <HTTPClient.h>

// Configuração do modelo AI-Thinker (ESP32-CAM padrão)
#define CAMERA_MODEL_AI_THINKER

#if defined(CAMERA_MODEL_AI_THINKER)
  #define PWDN_GPIO_NUM     32
  #define RESET_GPIO_NUM    -1
  #define XCLK_GPIO_NUM      0
  #define SIOD_GPIO_NUM     26
  #define SIOC_GPIO_NUM     27
  #define Y9_GPIO_NUM       35
  #define Y8_GPIO_NUM       34
  #define Y7_GPIO_NUM       39
  #define Y6_GPIO_NUM       36
  #define Y5_GPIO_NUM       21
  #define Y4_GPIO_NUM       19
  #define Y3_GPIO_NUM       18
  #define Y2_GPIO_NUM        5
  #define VSYNC_GPIO_NUM    25
  #define HREF_GPIO_NUM     23
  #define PCLK_GPIO_NUM     22
  #define LED_GPIO_NUM       4
#else
  #error "Defina o modelo correto da câmera!"
#endif

// Wi-Fi
const char* ssid = "HACHIKO_2.4Ghz";
const char* password = "wifi_livre2";

// URL para enviar as imagens
//const char* serverUrl = "http://webhook.site/f5daacd9-41cb-4fef-b64c-a960dd41f3e3";

//const char* serverUrl = "http://eoqloftvqs6oh8n.m.pipedream.net";

const char *serverUrl = "http://192.168.0.149:5000/";


void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println("Inicializando ESP32-CAM...");

  pinMode(LED_GPIO_NUM, OUTPUT);
  digitalWrite(LED_GPIO_NUM, LOW);

  // Configurações da câmera
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;

  // Qualidade baixa para reduzir uso de memória e processamento
  config.frame_size = FRAMESIZE_QVGA;  // 320x240
  config.jpeg_quality = 30;             // De 10 (máximo) a 63 (mínimo)
  config.fb_count = 1;
  config.fb_location = CAMERA_FB_IN_PSRAM;

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Erro ao iniciar câmera: 0x%x\n", err);
    while (true) delay(1000);
  }

  sensor_t *sensor = esp_camera_sensor_get();
  Serial.printf("Sensor detectado: PID = 0x%x\n", sensor->id.PID);
  if (sensor->id.PID != 0x2642) {
    Serial.println("ATENÇÃO: Sensor não é OV2640!");
  }

  // Conecta no Wi-Fi
  Serial.printf("Conectando-se ao Wi-Fi %s\n", ssid);
  WiFi.begin(ssid, password);
  WiFi.setSleep(false);

  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println();
  Serial.println("Wi-Fi conectado!");
  Serial.print("IP local: ");
  Serial.println(WiFi.localIP());
}

void loop() {
  Serial.println("Capturando imagem...");
  digitalWrite(LED_GPIO_NUM, HIGH);
  delay(100);

  camera_fb_t *fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("Falha ao capturar imagem");
    digitalWrite(LED_GPIO_NUM, LOW);
    delay(3000);
    return;
  }

  Serial.printf("Imagem capturada com %d bytes\n", fb->len);

  // Envia via HTTP POST para o servidor
  HTTPClient http;
  http.begin(serverUrl);
  http.addHeader("Content-Type", "image/jpeg");
  int httpResponseCode = http.POST(fb->buf, fb->len);

  // HTTPClient http;
  // http.begin(serverUrl);
  // int httpResponseCode = http.GET();

  if (httpResponseCode > 0) {
    Serial.printf("Imagem enviada com sucesso! Código: %d\n", httpResponseCode);
  } else {
    Serial.printf("Erro no envio: %s\n", http.errorToString(httpResponseCode).c_str());
  }

  http.end();
  esp_camera_fb_return(fb);
  digitalWrite(LED_GPIO_NUM, LOW);

  delay(10000);  // Espera 10 segundos para a próxima foto
}
```

// usar a lib esp32 by espressif systems na versao 2.0.11

#include <WiFi.h>
#include <BluetoothSerial.h>
#include "esp_camera.h"
#include <HTTPClient.h>
#include "SPIFFS.h"

#define LED_BUILTIN 4
#define LED_GPIO_NUM 4
// esse guid deve ser alterado para cada esp32 
const char *serverUrl = "http://incar.gsalute.com.br/upload.php?text=d7f3a9e2-8c4b-4a1f-9b6f-3e2c9e5d1a7b";
const char *path = "/c.txt";

BluetoothSerial SerialBT;
String ssid, password;
bool wifiConfigured = false;
bool wifiConnected = false;

// Pinos da câmera AI-Thinker
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
#else
#error "Defina o modelo correto da câmera!"
#endif

void blinkMorse(const String &morse) {
  for (char c : morse) {
    digitalWrite(LED_BUILTIN, HIGH);
    delay(c == '.' ? 100 : c == '-' ? 400 : 400);
    digitalWrite(LED_BUILTIN, LOW);
    delay(200);
  }
}

String sanitize(String text) {
  text.trim();
  text.replace("\r", "");
  text.replace("\n", "");
  return text;
}

void initCamera() {
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
  config.frame_size = FRAMESIZE_VGA;
  config.jpeg_quality = 5;
  config.fb_count = 1;
  config.fb_location = CAMERA_FB_IN_PSRAM;
  esp_log_level_set("*", ESP_LOG_NONE);
  if (esp_camera_init(&config) != ESP_OK) {
    Serial.println("Erro ao iniciar câmera");
    while (true) delay(1000);
  }
}

void readWiFiFromFile() {
  File fr = SPIFFS.open(path);
  String content = fr.readString();
  fr.close();

  int sepIndex = content.indexOf(';');
  if (sepIndex != -1) {
    ssid = sanitize(content.substring(0, sepIndex));
    password = sanitize(content.substring(sepIndex + 1));
    wifiConfigured = true;
  }
}

void saveWiFiToFile(const String &data) {
  File f = SPIFFS.open(path, FILE_WRITE);
  f.print(data);
  f.close();
}

bool connectWiFi() {
  WiFi.begin(ssid.c_str(), password.c_str());
  for (int i = 0; i < 10 && WiFi.status() != WL_CONNECTED; i++) {
    delay(1000);
    Serial.print(".");
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n Conectado");
    Serial.println("IP: " + WiFi.localIP().toString());
    blinkMorse("--- -.-"); // OK
    return true;
  } else {
    Serial.println("\n❌ Falha na conexão!");
    blinkMorse("... --- ..."); // SOS
    return false;
  }
}
void captureAndSendImage() {
  digitalWrite(LED_GPIO_NUM, HIGH);
  delay(100);

  camera_fb_t *fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("❌ Falha ao capturar imagem");
    digitalWrite(LED_GPIO_NUM, LOW);
    return;
  }

  HTTPClient http;
  String boundary = "----ESP32Boundary";
  String contentType = "multipart/form-data; boundary=" + boundary;
  http.begin(serverUrl);
  http.addHeader("Content-Type", contentType);

  // ⏱️ Aumenta o tempo de espera para 30 segundos
  http.setTimeout(30000); // 30.000 ms = 30 segundos

  // Cabeçalho do corpo
  String bodyStart = "--" + boundary + "\r\n";
  bodyStart += "Content-Disposition: form-data; name=\"image\"; filename=\"photo.jpg\"\r\n";
  bodyStart += "Content-Type: image/jpeg\r\n\r\n";

  String bodyEnd = "\r\n--" + boundary + "--\r\n";

  int totalLength = bodyStart.length() + fb->len + bodyEnd.length();
  uint8_t *body = (uint8_t *)malloc(totalLength);
  if (!body) {
    Serial.println("❌ Falha ao alocar memória");
    esp_camera_fb_return(fb);
    digitalWrite(LED_GPIO_NUM, LOW);
    return;
  }

  memcpy(body, bodyStart.c_str(), bodyStart.length());
  memcpy(body + bodyStart.length(), fb->buf, fb->len);
  memcpy(body + bodyStart.length() + fb->len, bodyEnd.c_str(), bodyEnd.length());

  int httpResponseCode = http.POST(body, totalLength);

  if (httpResponseCode > 0) {
    String response = http.getString();
    Serial.printf("✅ Imagem enviada! Código: %d\n", httpResponseCode);
    Serial.println("📨 Resposta do servidor: " + response);
  } else {
    Serial.printf("❌ Erro no envio: %s\n", http.errorToString(httpResponseCode).c_str());
  }

  free(body);
  http.end();
  esp_camera_fb_return(fb);
  digitalWrite(LED_GPIO_NUM, LOW);
}



void setup() {
  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(LED_GPIO_NUM, OUTPUT);
  digitalWrite(LED_GPIO_NUM, LOW);

  initCamera();

  if (!SPIFFS.begin(true)) {
    Serial.println("Erro ao montar SPIFFS");
    return;
  }

  if (SPIFFS.exists(path)) {
    readWiFiFromFile();
  }

  if (!wifiConfigured) {
    SerialBT.begin("ESP32CAM_BT");
    Serial.println("Aguardando dados via Bluetooth...");
  }
}

void loop() {
  if (!wifiConfigured && SerialBT.available()) {
    String received = SerialBT.readStringUntil('\n');
    int sepIndex = received.indexOf(';');
    if (sepIndex != -1) {
      ssid = sanitize(received.substring(0, sepIndex));
      password = sanitize(received.substring(sepIndex + 1));
      wifiConfigured = true;
      saveWiFiToFile(ssid + ";" + password);
      SerialBT.end();  // Desliga Bluetooth antes de tentar Wi-Fi
      btStop(); 
    }
  }

  if (wifiConfigured && !wifiConnected) {
    wifiConnected = connectWiFi();
    if (!wifiConnected) {
      WiFi.disconnect(true);
      delay(1000);
      SerialBT.begin("ESP32CAM_BT");  // Reativa Bluetooth se falhar
    }
  }

  if (wifiConnected) {
    captureAndSendImage();
  }
  delay(1000);
}

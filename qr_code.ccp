#include <WiFi.h>
#include "SPIFFS.h"
#include "esp_camera.h"
#include <quirc.h>

// ==================== PINOS AI-Thinker ====================
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM     0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM       5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

// ==================== CONFIG ====================
const char* WIFI_FILE = "/wifi.txt";
String ssid, senha;

// Task handle
TaskHandle_t qrTaskHandle = NULL;

// Quirc global
struct quirc *q;

// ==================== FUNÇÕES ====================
bool parseSsidSenha(const String &line, String &s, String &p) {
  int sep = line.indexOf(';');
  if (sep < 1) return false;
  s = line.substring(0, sep);
  p = line.substring(sep + 1);
  s.trim(); p.trim();
  return (s.length() > 0);
}

bool loadCredentials() {
  if (!SPIFFS.exists(WIFI_FILE)) return false;
  File f = SPIFFS.open(WIFI_FILE, FILE_READ);
  if (!f) return false;
  String line = f.readStringUntil('\n');
  f.close();
  return parseSsidSenha(line, ssid, senha);
}

bool saveCredentials(const String &s, const String &p) {
  File f = SPIFFS.open(WIFI_FILE, FILE_WRITE);
  if (!f) return false;
  f.printf("%s;%s\n", s.c_str(), p.c_str());
  f.close();
  return true;
}

bool connectWiFi(const String &s, const String &p) {
  WiFi.mode(WIFI_STA);
  WiFi.begin(s.c_str(), p.c_str());
  uint32_t start = millis();
  while (WiFi.status() != WL_CONNECTED && (millis() - start) < 15000) {
    delay(250);
    Serial.print('.');
  }
  Serial.println();
  return (WiFi.status() == WL_CONNECTED);
}

bool initCamera() {
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
  config.pixel_format = PIXFORMAT_GRAYSCALE;
  config.frame_size = FRAMESIZE_QQVGA; // 160x120
  config.fb_count = 1;
  config.fb_location = CAMERA_FB_IN_PSRAM;
  return (esp_camera_init(&config) == ESP_OK);
}

// ==================== TASK QR ====================
void qrTask(void *pvParameters) {
  camera_fb_t *fb;
  String qrText;

  while(true) {
    if (WiFi.status() != WL_CONNECTED) {
      fb = esp_camera_fb_get();
      if (fb) {
        int w = fb->width;
        int h = fb->height;
        if (quirc_resize(q, w, h) >= 0) {
          uint8_t *pixels = quirc_begin(q, &w, &h);
          memcpy(pixels, fb->buf, w*h);
          quirc_end(q);

          int count = quirc_count(q);
          for (int i=0;i<count;i++) {
            struct quirc_code code;
            struct quirc_data data;
            quirc_extract(q, i, &code);
            if (quirc_decode(&code, &data) == 0) {
              qrText = String((char*)data.payload);
              Serial.printf("QR: %s\n", qrText.c_str());
              String s,p;
              if(parseSsidSenha(qrText,s,p)) {
                if(connectWiFi(s,p)) {
                  saveCredentials(s,p);
                  Serial.println("Conectado e salvo!");
                }
              }
            }
          }
        }
        esp_camera_fb_return(fb);
      }
    }
    vTaskDelay(3000/portTICK_PERIOD_MS);
  }
}

// ==================== SETUP ====================
void setup() {
  Serial.begin(115200);
  delay(500);

  if(!SPIFFS.begin(true)) Serial.println("Erro SPIFFS");

  if(loadCredentials()) {
    Serial.printf("Tentando WiFi: %s / %s\n", ssid.c_str(), senha.c_str());
    if(connectWiFi(ssid,senha)) {
      Serial.println("Conectado com credenciais salvas.");
    }
  }

  if(!initCamera()) { Serial.println("Erro câmera"); while(1) delay(1000); }

  q = quirc_new();
  if(!q){ Serial.println("Erro quirc"); while(1) delay(1000); }

  // Cria a task QR com stack grande (16KB)
  xTaskCreatePinnedToCore(qrTask, "QR_Task", 16384, NULL, 1, &qrTaskHandle, 1);

  Serial.println("Setup finalizado. Task QR rodando...");
}

void loop() {
  // Loop principal leve
  delay(1000);
}


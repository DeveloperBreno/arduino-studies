#include <WiFi.h>
#include <WebSocketsClient.h>
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
const char* WIFI_FILE = "/w.txt";
String ssid_from_spiffs, pass_from_spiffs;

// WebSocket server configuration
const char* websocket_server = "192.168.0.16";
const uint16_t websocket_port = 8080;
const char* websocket_path = "/ws";

WebSocketsClient webSocket;

// Global flag to track WebSocket connection status
bool websocketConnected = false;

// Task handles
TaskHandle_t qrTaskHandle = NULL;
TaskHandle_t sendPhotoTaskHandle = NULL;

enum CameraMode { CAMERA_MODE_NONE, CAMERA_QR, CAMERA_PHOTO };
CameraMode currentCameraMode = CAMERA_MODE_NONE;

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
  return parseSsidSenha(line, ssid_from_spiffs, pass_from_spiffs);
}

bool saveCredentials(const String &s, const String &p) {
  File f = SPIFFS.open(WIFI_FILE, FILE_WRITE);
  if (!f) return false;
  f.printf("%s;%s\n", s.c_str(), p.c_str());
  f.close();
  return true;
}

bool connectWiFi(const String &s, const String &p, int timeout_ms = 15000) {
  WiFi.mode(WIFI_STA);
  WiFi.begin(s.c_str(), p.c_str());
  uint32_t start = millis();
  while (WiFi.status() != WL_CONNECTED && (millis() - start) < timeout_ms) {
    delay(250);
    Serial.print('.');
  }
  Serial.println();
  return (WiFi.status() == WL_CONNECTED);
}

void webSocketEvent(WStype_t type, uint8_t * payload, size_t length) {
  switch(type) {
    case WStype_CONNECTED:
      Serial.println("[WebSocket] Conectado ao servidor!");
      websocketConnected = true; // Set flag to true on connection
      break;
    case WStype_DISCONNECTED:
      Serial.println("[WebSocket] Desconectado do servidor!");
      websocketConnected = false; // Set flag to false on disconnection
      break;
    case WStype_TEXT:
      Serial.printf("[WebSocket] Mensagem de texto recebida: %s\n", payload);
      break;
    case WStype_BIN:
      Serial.printf("[WebSocket] Mensagem binária recebida, tamanho: %u\n", length);
      break;
    case WStype_ERROR:
      Serial.println("[WebSocket] Erro de WebSocket!");
      websocketConnected = false; // Set flag to false on error
      break;
    case WStype_FRAGMENT_TEXT_START:
    case WStype_FRAGMENT_BIN_START:
    case WStype_FRAGMENT:
    case WStype_FRAGMENT_FIN:
      break;
  }
}

bool initCameraQrCode() {
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

bool initCameraSendPhoto() {
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
  config.jpeg_quality = 10;
  config.fb_count = 1;
  config.fb_location = CAMERA_FB_IN_PSRAM;

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Erro ao inicializar a câmera para envio de fotos: 0x%x\n", err);
    return false;
  }
  return true;
}

void deinitCamera() {
  esp_camera_deinit();
}

// ==================== TASK QR ====================
void qrTask(void *pvParameters) {
  camera_fb_t *fb;
  String qrText;

  // Initialize camera for QR mode when task starts/resumes
  if(!initCameraQrCode()) {
    Serial.println("[QR Task] Erro ao inicializar câmera para QR na inicialização da task.");
    vTaskSuspend(NULL); // Suspend self if camera fails to initialize
  }
  currentCameraMode = CAMERA_QR;
  Serial.println("[QR Task] Câmera inicializada para modo QR.");

  while(true) {
    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("[QR Task] WiFi não conectado. Tentando ler QR Code...");
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
                  // De-initialize camera and activate photo task
                  deinitCamera();
                  currentCameraMode = CAMERA_MODE_NONE;
                  vTaskResume(sendPhotoTaskHandle);
                  vTaskSuspend(NULL); // Suspend self
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

// ==================== TASK SEND PHOTO ====================
void sendPhotoTask(void *pvParameters) {
  camera_fb_t * fb = NULL;
  static unsigned long lastWebSocketAttempt = 0;
  static unsigned long lastPhotoSendTime = 0; // New static variable for photo sending frequency
  const unsigned long PHOTO_SEND_INTERVAL_MS = 3000; // 3 seconds

  // Initialize camera for photo mode when task starts/resumes
  if(!initCameraSendPhoto()) {
    Serial.println("[Send Photo Task] Erro ao inicializar câmera para foto na inicialização da task.");
    vTaskSuspend(NULL); // Suspend self if camera fails to initialize
  }
  currentCameraMode = CAMERA_PHOTO;
  Serial.println("[Send Photo Task] Câmera inicializada para modo foto.");

  while(true) {
    if (WiFi.status() == WL_CONNECTED) {
      webSocket.loop(); // Always call loop to process events and maintain connection

      if (!websocketConnected && millis() - lastWebSocketAttempt > 5000) {
        lastWebSocketAttempt = millis();
        Serial.println("[WebSocket] Tentando reconectar...");
        webSocket.begin(websocket_server, websocket_port, websocket_path);
        webSocket.onEvent(webSocketEvent);
      }
      
      if(websocketConnected) {
        // Only send photo if PHOTO_SEND_INTERVAL_MS has passed since last send
        if (millis() - lastPhotoSendTime > PHOTO_SEND_INTERVAL_MS) {
          fb = esp_camera_fb_get();
          if(fb) {
            Serial.printf("[Send Photo Task] Enviando imagem, tamanho: %zu bytes...\n", fb->len);
            bool sent = webSocket.sendBIN(fb->buf, fb->len);
            if(sent) {
              Serial.println("[Send Photo Task] Imagem enviada com sucesso!");
              lastPhotoSendTime = millis(); // Update last send time only on successful send
            } else {
              Serial.println("[Send Photo Task] Falha ao enviar imagem!");
            }
            esp_camera_fb_return(fb);
          } else {
            Serial.println("[Send Photo Task] Falha ao capturar imagem");
          }
        }
      } else {
        Serial.println("[Send Photo Task] WebSocket não conectado, aguardando...");
      }
      vTaskDelay(100 / portTICK_PERIOD_MS); // Ensure task yields and WebSocket loop runs frequently
    } else {
      Serial.println("[Send Photo Task] WiFi não conectado, aguardando Task QR...");
      deinitCamera();
      currentCameraMode = CAMERA_MODE_NONE;
      vTaskResume(qrTaskHandle);
      vTaskSuspend(NULL); // Suspend self
    }
  }
}

// ==================== SETUP ====================
void setup() {
  Serial.begin(115200);
  delay(500);

  if(!SPIFFS.begin(true)) {
    Serial.println("Erro SPIFFS");
    while(true);
  }

  bool wifiConnected = false;

  if(loadCredentials()) {
    Serial.printf("Tentando WiFi com credenciais salvas: %s / %s\n", ssid_from_spiffs.c_str(), pass_from_spiffs.c_str());
    if(connectWiFi(ssid_from_spiffs, pass_from_spiffs)) {
      Serial.println("Conectado com credenciais salvas.");
      wifiConnected = true;
    } else {
      Serial.println("Falha ao conectar com credenciais salvas. Partindo para QR Code.");
      deinitCamera(); // Ensure camera is deinitialized for potential QR scan
    }
  } else {
    Serial.println("Nenhuma credencial WiFi salva encontrada. Partindo para QR Code.");
    deinitCamera(); // Ensure camera is deinitialized for potential QR scan
  }

  q = quirc_new();
  if(!q){ 
    Serial.println("Erro quirc"); 
    while(true); 
  }

  // Cria as tasks
  xTaskCreatePinnedToCore(qrTask, "QR_Task", 16384, NULL, 1, &qrTaskHandle, 1);
  xTaskCreatePinnedToCore(sendPhotoTask, "SendPhoto_Task", 16384, NULL, 1, &sendPhotoTaskHandle, 1);

  if (wifiConnected) {
    // If WiFi is connected, suspend QR task and let sendPhotoTask run
    vTaskSuspend(qrTaskHandle);
    vTaskResume(sendPhotoTaskHandle); // Ensure it starts if it was suspended before
    webSocket.begin(websocket_server, websocket_port, websocket_path);
    webSocket.onEvent(webSocketEvent);
  } else {
    // If WiFi is not connected, suspend sendPhotoTask and let QR task run
    vTaskSuspend(sendPhotoTaskHandle);
    vTaskResume(qrTaskHandle); // Ensure it starts if it was suspended before
  }

  Serial.println("Setup finalizado.");
}

void loop() {
  // Loop principal leve, as tarefas são gerenciadas pelas FreeRTOS tasks
  delay(1000);
}


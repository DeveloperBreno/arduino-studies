#include <WiFi.h>
#include <WebSocketsClient.h>
#include "esp_camera.h"

// --- CONFIGURAÇÃO DO WIFI ---
const char* ssid = "HACHIKO_2.4Ghz";
const char* password = "wifi_livre2";

// --- CONFIGURAÇÃO DO SERVIDOR WEBSOCKET ---
const char* websocket_server = "192.168.0.16";
const uint16_t websocket_port = 8080;
const char* websocket_path = "/ws";

WebSocketsClient webSocket;

// --- CONFIGURAÇÃO DA CÂMERA AI-THINKER ---
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

void setupCamera() {
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
    Serial.printf("Erro ao inicializar a câmera: 0x%x\n", err);
    while(true);
  }
}

// --- FUNÇÃO DE CALLBACK DO WEBSOCKET ---
void webSocketEvent(WStype_t type, uint8_t * payload, size_t length) {
  switch(type) {
    case WStype_CONNECTED:
      Serial.println("[WebSocket] Conectado ao servidor!");
      break;
    case WStype_DISCONNECTED:
      Serial.println("[WebSocket] Desconectado do servidor!");
      break;
    case WStype_TEXT:
      Serial.printf("[WebSocket] Mensagem de texto recebida: %s\n", payload);
      break;
    case WStype_BIN:
      Serial.printf("[WebSocket] Mensagem binária recebida, tamanho: %u\n", length);
      break;
    case WStype_ERROR:
      Serial.println("[WebSocket] Erro de WebSocket!");
      break;
    case WStype_FRAGMENT_TEXT_START:
    case WStype_FRAGMENT_BIN_START:
    case WStype_FRAGMENT:
    case WStype_FRAGMENT_FIN:
      break;
  }
}

// --- TENTA CONECTAR AO WIFI COM TIMEOUT DE 10 SEGUNDOS ---
bool connectWiFi() {
  Serial.printf("[Wi-Fi] Conectando à rede: %s\n", ssid);
  WiFi.begin(ssid, password);

  unsigned long startAttemptTime = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 10000) {
    delay(200);
    Serial.print(".");
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n[Wi-Fi] Conectado!");
    Serial.print("[Wi-Fi] IP: ");
    Serial.println(WiFi.localIP());
    return true;
  } else {
    Serial.println("\n[Wi-Fi] Falha ao conectar. Tentaremos novamente depois.");
    return false;
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("[Setup] Inicializando câmera...");
  setupCamera();

  Serial.println("[Setup] Conectando Wi-Fi...");
  connectWiFi();

  Serial.println("[Setup] Inicializando WebSocket...");
  webSocket.begin(websocket_server, websocket_port, websocket_path);
  webSocket.onEvent(webSocketEvent);
}

void loop() {
  static unsigned long lastWiFiAttempt = 0;
  static unsigned long lastWebSocketAttempt = 0;

  // Reconectar Wi-Fi se caiu (10s)
  if(WiFi.status() != WL_CONNECTED && millis() - lastWiFiAttempt > 10000) {
    lastWiFiAttempt = millis();
    connectWiFi();
  }

  // Loop WebSocket
  webSocket.loop();

  // Reconectar WebSocket se desconectado (5s)
  if(!webSocket.isConnected() && millis() - lastWebSocketAttempt > 5000) {
    lastWebSocketAttempt = millis();
    Serial.println("[WebSocket] Tentando reconectar...");
    webSocket.begin(websocket_server, websocket_port, websocket_path);
    webSocket.onEvent(webSocketEvent);
  }

  // Só envia imagem se Wi-Fi e WebSocket estiverem conectados
  if(WiFi.status() == WL_CONNECTED && webSocket.isConnected()) {
    camera_fb_t * fb = esp_camera_fb_get();
    if(fb) {
      Serial.printf("[Loop] Enviando imagem, tamanho: %zu bytes...\n", fb->len);
      bool sent = webSocket.sendBIN(fb->buf, fb->len);
      if(sent) {
        Serial.println("[Loop] Imagem enviada com sucesso!");
      } else {
        Serial.println("[Loop] Falha ao enviar imagem!");
      }
      esp_camera_fb_return(fb);
    } else {
      Serial.println("[Loop] Falha ao capturar imagem");
    }

    delay(3000); // Aguarda 3 segundos entre envios
  }
}

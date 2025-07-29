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


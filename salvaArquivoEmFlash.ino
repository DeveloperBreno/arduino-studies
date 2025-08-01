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

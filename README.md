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


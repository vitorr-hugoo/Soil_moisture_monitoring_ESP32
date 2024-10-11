/*------- Código desenvolvido para um sistema de irrigação caseira com foco no cultivo de morangos-------
  ------- Vitor Hugo Pavanelli dos Reis -----------------------------------------------------------------
  ------- Versão 8 - 30/09/2024 -------------------------------------------------------------------------
  ------- Versão Final ---------------------------------------------------------------------
*/
//---------Bibliotecas
#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <esp_task_wdt.h>

/*---------------- Lógica -------------------------------------------------------------------------------------------------------
-> 4 níveis: encharcado, adequado, submolhado, seco

      encharcado  400 - 1199  -> se colocar 0 pode significar que não ta lendo direito
      Adequado   1200 - 1399
      Submolhado 1400 - 1999
      Solo seco  2000+
---------------------------------------------------------------------------------------------------------------------------------*/

//--------------- Variáveis Globais------------------------------------------------------------------------------------------------
int valor_sensor_1=0;
int valor_sensor_2=0;
int valor_sensor_3=0;
int valor_sensor_4=0;
int media_sensores=0;

//--------------- Variáveis dos Sensores ------------------------------------------------------------------------------------------

const int pino_sensor_1 = 32; // Sensor 1
const int pino_sensor_2 = 33; // Sensor 2
const int pino_sensor_3 = 36; // Sensor 3
const int pino_sensor_4 = 39; // Sensor 4

//Wifi config
const char* ssid = "xxxxxxxxxxxxxx"; //ssid
const char* password = "xxxxxxxxxx"; //password

//IP config 
IPAddress local_IP(10, 0, 0, 123);
IPAddress gateway(10, 0, 0, 1);
IPAddress subnet(255, 255, 255, 0);


//Configura a porta do servidor -> porta 80
WebServer server(80);

// Estrutura de configuração para o Watchdog Timer
esp_task_wdt_config_t wdt_config;

void configuraWatchdog(int timeoutSegundos) {
  // Configuração da estrutura esp_task_wdt_config_t
  wdt_config.timeout_ms = timeoutSegundos * 1000;  // Converte segundos para milissegundos
  wdt_config.idle_core_mask = 0;                   // Não monitorar tarefas ociosas
  wdt_config.trigger_panic = true;                 // Causa um pânico se o timeout ocorrer

  // Inicializa o Watchdog com a configuração especificada
  esp_task_wdt_init(&wdt_config);
  esp_task_wdt_add(NULL);  // Adiciona a tarefa principal (loop) ao Watchdog
  //Serial.println("Watchdog configurado com timeout de " + String(timeoutSegundos) + " segundos");
}

void alimentaWatchdog() {
  esp_task_wdt_reset();  // Reseta o Watchdog Timer para evitar que ele reinicie o ESP32
  //Serial.println("Watchdog alimentado");
  //delay(5000);
}

void handleRoot() {
  char msg[1500];
  snprintf(msg, 1500,
    "<!DOCTYPE html><html lang='pt-BR'><head><meta charset='UTF-8'>"
    "<meta name='viewport' content='width=device-width, initial-scale=1.0'>"
    "<meta http-equiv='refresh' content='5'>"
    "<title>Monitoramento de Umidade de Solo via ESP32</title>"
    "<style>* { margin: 0; padding: 0; box-sizing: border-box; }"
    "body { font-family: Arial, sans-serif; display: flex; justify-content: center; align-items: center; height: 100vh; background-color: #f4f4f4; }"
    ".container { text-align: center; background-color: #fff; padding: 20px; border-radius: 8px; box-shadow: 0 4px 8px rgba(0, 0, 0, 0.1); }"
    "h1 { background-color: #ccc; padding: 10px; margin-bottom: 20px; }"
    ".box { background-color: #e0e0e0; padding: 15px; margin: 10px 0; border-radius: 5px; }"
    ".box-black { background-color: #000; color: #fff; padding: 15px; margin: 10px 0; border-radius: 5px; }"
    "p { margin: 0; font-size: 16px; }</style></head>"
    "<body><div class='container'><h1>Monitoramento de umidade de solo via ESP32</h1>"
    "<div class='box-black'><p>Leitura Analógica dos dados</p></div>"
    "<div class='box'><p>%d</p></div>"
    "<div class='box-black'><p>Status do solo</p></div>"
    "<div class='box'><p>%s</p></div>"
    "<div class='box-black'><p>Umidade percentual</p></div>"
    "<div class='box'><p>%.2f %%</p></div></div></body></html>", 
    media_sensores, statusSensor(), aquisicaoUmidade());

  server.send(200, "text/html", msg);
}

void aquisicaoDados() {
  valor_sensor_1 = analogRead(pino_sensor_1);  // lê o valor do sensor
  delay(5);
  valor_sensor_2 = analogRead(pino_sensor_2);
  delay(5);
  valor_sensor_3 = analogRead(pino_sensor_3);
  delay(5);
  valor_sensor_4 = analogRead(pino_sensor_4);
  delay(5);
  /*Serial.println("-------------------------------------------------");
  Serial.println(valor_sensor_1);
  Serial.println(valor_sensor_2);
  Serial.println(valor_sensor_3);
  Serial.println(valor_sensor_4);
  Serial.println("-------------------------------------------------");*/
}

int media(int sensor1, int sensor2, int sensor3, int sensor4){
  media_sensores=(sensor1 + sensor2 + sensor3 + sensor4)/4;
  //Serial.println(media_sensores);
  return media_sensores;
}

String statusSensor() {
  if (media_sensores >= 2000) return "Seco";
  if (media_sensores > 1400) return "Submolhado";
  if (media_sensores > 1200) return "Adequado";
  if (media_sensores > 400) return "Encharcado";
  return "Problemas";
}

float aquisicaoUmidade() {
  return map(media_sensores, 1000, 3100, 900, 0) / 10.0;
}

void setup() {
  Serial.begin(115200);
  pinMode(pino_sensor_1, INPUT);
  pinMode(pino_sensor_2, INPUT);
  pinMode(pino_sensor_3, INPUT);
  pinMode(pino_sensor_4, INPUT);

  configuraWatchdog(15);  // Configura o Watchdog Timer com timeout de 15 segundos

  WiFi.mode(WIFI_STA);
  WiFi.config(local_IP, gateway, subnet);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("Connected to WiFi");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  if (MDNS.begin("esp32")) {
    Serial.println("MDNS responder started");
  }

  server.on("/", handleRoot);
  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  aquisicaoDados();
  media(valor_sensor_1,valor_sensor_2, valor_sensor_3, valor_sensor_4);
  server.handleClient();  // Lida com requisições HTTP
  delay(2);               // Pequeno atraso para evitar consumo excessivo de CPU
  alimentaWatchdog();      // Alimenta o Watchdog Timer
}

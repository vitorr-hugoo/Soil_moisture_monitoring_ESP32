/*------- Código desenvolvido para um sistema de irrigação caseira com foco no cultivo de morangos-------
  ------- Vitor Hugo Pavanelli dos Reis -----------------------------------------------------------------
  ------- Versão 9 - 30/09/2024 -------------------------------------------------------------------------
  ------- Versão Casa da vó ---------------------------------------------------------------------
*/
//---------Bibliotecas
#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include "esp_task_wdt.h"

/*---------------- Lógica -------------------------------------------------------------------------------------------------------
-> 4 níveis: encharcado, adequado, submolhado, seco

      encharcado  500 - 1049  -> se colocar 0 pode significar que não ta lendo direito
      Adequado   1050 - 1399
      Submolhado 1400 - 1899
      Solo seco  1900 - 3200
---------------------------------------------------------------------------------------------------------------------------------*/

//--------------- Variáveis Globais------------------------------------------------------------------------------------------------
int valor_sensor_1 = 0;
int valor_sensor_2 = 0;
int valor_sensor_3 = 0;
int valor_sensor_4 = 0;
int media_sensores = 0;

//--------------- Variáveis dos Sensores ------------------------------------------------------------------------------------------

const int pino_sensor_1 = 32;
const int pino_sensor_2 = 33;
const int pino_sensor_3 = 36;
const int pino_sensor_4 = 39;

//-------------- Declaração de funções -----------------------------------------------------------------------------------------------
void aquisicaoDados();
int media(int sensor1, int sensor2, int sensor3, int sensor4);
String statusSensor();
float aquisicaoUmidade();
void configuraWatchdog(int timeoutSegundos);
void alimentaWatchdog();
void handleRoot();

//-------------- Configurações WIFI ----------------------------------------------------------------------------------------------------
const char *ssid = "xxxxxxxx";
const char *password = "xxxxxxxx";

IPAddress local_IP(192, 168, 0, 123);
IPAddress gateway(192, 168, 0, 1);
IPAddress subnet(255, 255, 255, 0);

WebServer server(80); // PORTA 80 -> PARA CONSEGUIR ACESSAR SEM ESTAR NA REDE LOCAL, MEXER NAS CONFIGURAÇÕES DE PORTA DO ROTEADOR

// SETUP
void setup()
{
  Serial.begin(115200);
  pinMode(pino_sensor_1, INPUT);
  pinMode(pino_sensor_2, INPUT);
  pinMode(pino_sensor_3, INPUT);
  pinMode(pino_sensor_4, INPUT);

  configuraWatchdog(15); // Configura o Watchdog Timer com timeout de 15 segundos

  // INICIA E CONFIGURA WIFI
  WiFi.mode(WIFI_STA);
  WiFi.config(local_IP, gateway, subnet);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  Serial.println("Connected to WiFi");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  if (MDNS.begin("esp32"))
  {
    Serial.println("MDNS responder started");
  }

  server.on("/", handleRoot);
  server.begin();
  Serial.println("HTTP server started");
}

// LOOP
void loop()
{
  aquisicaoDados();
  media(valor_sensor_1, valor_sensor_2, valor_sensor_3, valor_sensor_4);
  server.handleClient(); // Lida com requisições HTTP
  delay(2);              // Pequeno atraso para evitar consumo excessivo de CPU
  alimentaWatchdog();    // Alimenta o Watchdog Timer
}

// FUNÇÃO QUE FAZ A LEITURA ANALOGICA DO SENSOR
void aquisicaoDados()
{
  valor_sensor_1 = analogRead(pino_sensor_1);
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
  Serial.println("-------------------------------------------------");
  */
}

// CALCULA A MÉDIA DOS DADOS DOS SENSORES
int media(int sensor1, int sensor2, int sensor3, int sensor4)
{
  media_sensores = (sensor1 + sensor2 + sensor3 + sensor4) / 4;
  return media_sensores;
}

// FUNÇÃO PARA VISUALIZAÇÃO DO STATUS DO SOLO
String statusSensor()
{
  if (media_sensores >= 1900)
    return "Seco";
  if (media_sensores > 1399)
    return "Submolhado";
  if (media_sensores > 1049)
    return "Adequado";
  if (media_sensores > 499)
    return "Encharcado";
  return "Problemas";
}

// CALCULA A UMIDADE PERCENTUAL
float aquisicaoUmidade()
{
  return map(media_sensores, 800, 3200, 100, 0);
}

// CONFIGURA O WATCHDOG
void configuraWatchdog(int timeoutSegundos)
{
  esp_task_wdt_init(timeoutSegundos * 1000, true); // Configura o watchdog com o timeout
  esp_task_wdt_add(NULL);                          // Adiciona a tarefa principal (loop) ao Watchdog
  // Serial.println("Watchdog configurado com timeout de " + String(timeoutSegundos) + " segundos");
}

void alimentaWatchdog()
{
  esp_task_wdt_reset(); // Reseta o Watchdog Timer para evitar que ele reinicie o ESP32
}

// WEB SERVER CRIADO PARA VISUALIZAÇÃO DOS DADOS VIA ENDEREÇO IP
void handleRoot()
{
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
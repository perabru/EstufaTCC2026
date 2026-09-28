// ============================================================
// ESP32 - ESTUFA / IRRIGACAO COM HIVEMQ
// ============================================================
//
// Sensores:
// - DHT11: temperatura e umidade do ar
// - Sensor de umidade do solo FC-28
//
// Atuador:
// - Rele 5V
// - Bomba 12V
//
// MQTT:
// - HiveMQ Cloud
//
// ------------------------------------------------------------
// PINAGEM
//
// FC-28 AO  -> GPIO 34
// DHT11     -> GPIO 26
// Rele IN   -> GPIO 23
//
// ============================================================

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <DHT.h>

// ============================================================
// 1. WIFI
// ============================================================

const char* WIFI_SSID  = "SUA_REDE_WIFI";
const char* WIFI_SENHA = "SUA_SENHA_WIFI";

// ============================================================
// 2. HIVEMQ CLOUD
// ============================================================

const char* MQTT_SERVIDOR =
  "d5010d2de7ff4182bd09c7fde4c243e5.s1.eu.hivemq.cloud";

const int MQTT_PORTA = 8883;

const char* MQTT_USUARIO = "thalita";
const char* MQTT_SENHA   = "123456789";

// ============================================================
// 3. PINAGEM
// ============================================================

#define PINO_SOLO 34
#define PINO_DHT 26
#define PINO_RELE 23

#define TIPO_DHT DHT11

// ============================================================
// 4. RELE
// ============================================================

// A maioria dos modulos de rele trabalha em LOW.
//
// LOW  = ligado
// HIGH = desligado

#define RELE_LIGADO LOW
#define RELE_DESLIGADO HIGH

bool bombaLigada = false;

// ============================================================
// 5. SENSOR DHT
// ============================================================

DHT dht(PINO_DHT, TIPO_DHT);

// ============================================================
// 6. CALIBRACAO SENSOR DE SOLO
// ============================================================
//
// AJUSTE estes valores de acordo com o seu sensor.
//
// Exemplo:
// Seco    = 3500
// Molhado = 1300
//
// Veja a leitura no Monitor Serial para calibrar.

int VALOR_SOLO_SECO = 3500;
int VALOR_SOLO_MOLHADO = 1300;

// ============================================================
// 7. TOPICOS MQTT
// ============================================================

// ---------- Sensores ----------

const char* TOPICO_TEMPERATURA =
  "estufa/sensor/temperatura";

const char* TOPICO_UMIDADE_AR =
  "estufa/sensor/umidade_ar";

const char* TOPICO_UMIDADE_SOLO =
  "estufa/sensor/umidade_solo";

const char* TOPICO_SOLO_ADC =
  "estufa/sensor/solo_adc";

// ---------- Bomba ----------

const char* TOPICO_BOMBA_COMANDO =
  "estufa/bomba/comando";

const char* TOPICO_BOMBA_STATUS =
  "estufa/bomba/status";

// ---------- ESP32 ----------

const char* TOPICO_ESP_STATUS =
  "estufa/esp/status";

const char* TOPICO_RSSI =
  "estufa/esp/rssi";

const char* TOPICO_UPTIME =
  "estufa/esp/uptime";

// ---------- Dados completos ----------

const char* TOPICO_DADOS =
  "estufa/dados";

// ============================================================
// 8. MQTT
// ============================================================

WiFiClientSecure wifiClient;
PubSubClient mqtt(wifiClient);

// ============================================================
// 9. TEMPORIZADORES
// ============================================================

unsigned long ultimoEnvio = 0;

// Envia dados a cada 5 segundos
const unsigned long INTERVALO_ENVIO = 5000;

// ============================================================
// 10. LIGAR BOMBA
// ============================================================

void ligarBomba() {

  digitalWrite(PINO_RELE, RELE_LIGADO);

  bombaLigada = true;

  Serial.println();
  Serial.println("==============================");
  Serial.println(">>> BOMBA LIGADA");
  Serial.println("==============================");

  if (mqtt.connected()) {

    mqtt.publish(
      TOPICO_BOMBA_STATUS,
      "LIGADA",
      true
    );
  }
}

// ============================================================
// 11. DESLIGAR BOMBA
// ============================================================

void desligarBomba() {

  digitalWrite(PINO_RELE, RELE_DESLIGADO);

  bombaLigada = false;

  Serial.println();
  Serial.println("==============================");
  Serial.println(">>> BOMBA DESLIGADA");
  Serial.println("==============================");

  if (mqtt.connected()) {

    mqtt.publish(
      TOPICO_BOMBA_STATUS,
      "DESLIGADA",
      true
    );
  }
}

// ============================================================
// 12. LEITURA DO SOLO
// ============================================================

int lerSoloADC() {

  long soma = 0;

  const int quantidadeLeituras = 10;

  for (int i = 0; i < quantidadeLeituras; i++) {

    soma += analogRead(PINO_SOLO);

    delay(5);
  }

  return soma / quantidadeLeituras;
}

// ============================================================
// 13. CONVERTER SOLO PARA PORCENTAGEM
// ============================================================

int calcularUmidadeSolo(int valorADC) {

  int porcentagem = map(
    valorADC,
    VALOR_SOLO_SECO,
    VALOR_SOLO_MOLHADO,
    0,
    100
  );

  porcentagem = constrain(
    porcentagem,
    0,
    100
  );

  return porcentagem;
}

// ============================================================
// 14. CALLBACK MQTT
// ============================================================
//
// Esta funcao recebe mensagens enviadas pelo celular.
//
// Topico:
//
// estufa/bomba/comando
//
// Mensagens aceitas:
//
// LIGAR
// DESLIGAR
//
// Tambem:
// ON
// OFF
// 1
// 0
//
// ============================================================

void callback(
  char* topic,
  byte* payload,
  unsigned int length
) {

  String mensagem = "";

  for (unsigned int i = 0; i < length; i++) {

    mensagem += (char)payload[i];
  }

  mensagem.trim();
  mensagem.toUpperCase();

  Serial.println();
  Serial.println("========== MQTT RECEBIDO ==========");

  Serial.print("Topico: ");
  Serial.println(topic);

  Serial.print("Mensagem: ");
  Serial.println(mensagem);

  Serial.println("===================================");

  // ==========================================================
  // COMANDO DA BOMBA
  // ==========================================================

  if (String(topic) == TOPICO_BOMBA_COMANDO) {

    if (
      mensagem == "LIGAR" ||
      mensagem == "ON" ||
      mensagem == "1"
    ) {

      ligarBomba();
    }

    else if (
      mensagem == "DESLIGAR" ||
      mensagem == "OFF" ||
      mensagem == "0"
    ) {

      desligarBomba();
    }

    else {

      Serial.println("Comando MQTT desconhecido.");
    }
  }
}

// ============================================================
// 15. CONECTAR AO WIFI
// ============================================================

void conectarWiFi() {

  if (WiFi.status() == WL_CONNECTED) {
    return;
  }

  Serial.println();
  Serial.println("==============================");
  Serial.println("CONECTANDO AO WIFI");
  Serial.println("==============================");

  Serial.print("Rede: ");
  Serial.println(WIFI_SSID);

  WiFi.mode(WIFI_STA);

  WiFi.begin(
    WIFI_SSID,
    WIFI_SENHA
  );

  int tentativas = 0;

  while (
    WiFi.status() != WL_CONNECTED &&
    tentativas < 40
  ) {

    delay(500);

    Serial.print(".");

    tentativas++;
  }

  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {

    Serial.println();
    Serial.println("WIFI CONECTADO!");

    Serial.print("IP: ");
    Serial.println(WiFi.localIP());

    Serial.print("RSSI: ");
    Serial.print(WiFi.RSSI());
    Serial.println(" dBm");
  }

  else {

    Serial.println("Falha ao conectar no WiFi.");
  }
}

// ============================================================
// 16. CONECTAR AO HIVEMQ
// ============================================================

void conectarMQTT() {

  if (mqtt.connected()) {
    return;
  }

  Serial.println();
  Serial.println("==============================");
  Serial.println("CONECTANDO AO HIVEMQ");
  Serial.println("==============================");

  String clienteID = "ESP32-ESTUFA-";

  clienteID += String(
    (uint32_t)(ESP.getEfuseMac() & 0xFFFFFFFF),
    HEX
  );

  Serial.print("Cliente MQTT: ");
  Serial.println(clienteID);

  bool conectado = mqtt.connect(
    clienteID.c_str(),
    MQTT_USUARIO,
    MQTT_SENHA,

    // Last Will Topic
    TOPICO_ESP_STATUS,

    // QoS
    1,

    // Retain
    true,

    // Mensagem caso caia
    "OFFLINE"
  );

  if (conectado) {

    Serial.println("HiveMQ conectado!");

    // ESP online
    mqtt.publish(
      TOPICO_ESP_STATUS,
      "ONLINE",
      true
    );

    // Estado atual da bomba
    mqtt.publish(
      TOPICO_BOMBA_STATUS,
      bombaLigada ? "LIGADA" : "DESLIGADA",
      true
    );

    // Recebe comandos
    mqtt.subscribe(
      TOPICO_BOMBA_COMANDO
    );

    Serial.print("Inscrito em: ");
    Serial.println(TOPICO_BOMBA_COMANDO);
  }

  else {

    Serial.print("Erro MQTT: ");
    Serial.println(mqtt.state());
  }
}

// ============================================================
// 17. ENVIAR DADOS PARA O HIVEMQ
// ============================================================

void enviarDados() {

  // ----------------------------------------------------------
  // DHT11
  // ----------------------------------------------------------

  float temperatura = dht.readTemperature();
  float umidadeAr = dht.readHumidity();

  // ----------------------------------------------------------
  // SOLO
  // ----------------------------------------------------------

  int soloADC = lerSoloADC();

  int umidadeSolo =
    calcularUmidadeSolo(soloADC);

  // ----------------------------------------------------------
  // SERIAL
  // ----------------------------------------------------------

  Serial.println();
  Serial.println("========================================");
  Serial.println("            LEITURA SENSORES");
  Serial.println("========================================");

  // Temperatura

  if (!isnan(temperatura)) {

    Serial.print("Temperatura: ");
    Serial.print(temperatura, 1);
    Serial.println(" C");
  }

  else {

    Serial.println("Temperatura: ERRO DHT11");
  }

  // Umidade do ar

  if (!isnan(umidadeAr)) {

    Serial.print("Umidade do ar: ");
    Serial.print(umidadeAr, 1);
    Serial.println(" %");
  }

  else {

    Serial.println("Umidade do ar: ERRO DHT11");
  }

  // Solo

  Serial.print("Solo ADC: ");
  Serial.println(soloADC);

  Serial.print("Umidade do solo: ");
  Serial.print(umidadeSolo);
  Serial.println(" %");

  // Bomba

  Serial.print("Bomba: ");

  if (bombaLigada) {

    Serial.println("LIGADA");

  }

  else {

    Serial.println("DESLIGADA");
  }

  // WiFi

  Serial.print("WiFi RSSI: ");
  Serial.print(WiFi.RSSI());
  Serial.println(" dBm");

  Serial.println("========================================");

  // ==========================================================
  // PUBLICAR MQTT
  // ==========================================================

  if (!mqtt.connected()) {

    Serial.println("MQTT desconectado.");

    return;
  }

  // ----------------------------------------------------------
  // TEMPERATURA
  // ----------------------------------------------------------

  if (!isnan(temperatura)) {

    char tempString[10];

    dtostrf(
      temperatura,
      1,
      1,
      tempString
    );

    mqtt.publish(
      TOPICO_TEMPERATURA,
      tempString,
      true
    );
  }

  // ----------------------------------------------------------
  // UMIDADE DO AR
  // ----------------------------------------------------------

  if (!isnan(umidadeAr)) {

    char umidadeString[10];

    dtostrf(
      umidadeAr,
      1,
      1,
      umidadeString
    );

    mqtt.publish(
      TOPICO_UMIDADE_AR,
      umidadeString,
      true
    );
  }

  // ----------------------------------------------------------
  // UMIDADE DO SOLO
  // ----------------------------------------------------------

  String soloString =
    String(umidadeSolo);

  mqtt.publish(
    TOPICO_UMIDADE_SOLO,
    soloString.c_str(),
    true
  );

  // ----------------------------------------------------------
  // ADC SOLO
  // ----------------------------------------------------------

  String adcString =
    String(soloADC);

  mqtt.publish(
    TOPICO_SOLO_ADC,
    adcString.c_str(),
    true
  );

  // ----------------------------------------------------------
  // BOMBA
  // ----------------------------------------------------------

  mqtt.publish(
    TOPICO_BOMBA_STATUS,
    bombaLigada ? "LIGADA" : "DESLIGADA",
    true
  );

  // ----------------------------------------------------------
  // RSSI
  // ----------------------------------------------------------

  String rssiString =
    String(WiFi.RSSI());

  mqtt.publish(
    TOPICO_RSSI,
    rssiString.c_str(),
    true
  );

  // ----------------------------------------------------------
  // UPTIME
  // ----------------------------------------------------------

  unsigned long uptime =
    millis() / 1000;

  String uptimeString =
    String(uptime);

  mqtt.publish(
    TOPICO_UPTIME,
    uptimeString.c_str(),
    true
  );

  // ==========================================================
  // JSON COM TODOS OS DADOS
  // ==========================================================

  String json = "{";

  // Temperatura

  json += "\"temperatura\":";

  if (!isnan(temperatura)) {

    json += String(temperatura, 1);

  }

  else {

    json += "null";
  }

  // Umidade ar

  json += ",\"umidade_ar\":";

  if (!isnan(umidadeAr)) {

    json += String(umidadeAr, 1);

  }

  else {

    json += "null";
  }

  // Solo %

  json += ",\"umidade_solo\":";
  json += String(umidadeSolo);

  // Solo ADC

  json += ",\"solo_adc\":";
  json += String(soloADC);

  // Bomba

  json += ",\"bomba\":\"";

  if (bombaLigada) {

    json += "LIGADA";

  }

  else {

    json += "DESLIGADA";
  }

  json += "\"";

  // RSSI

  json += ",\"rssi\":";
  json += String(WiFi.RSSI());

  // Uptime

  json += ",\"uptime\":";
  json += String(uptime);

  json += "}";

  // ----------------------------------------------------------
  // PUBLICAR JSON
  // ----------------------------------------------------------

  mqtt.publish(
    TOPICO_DADOS,
    json.c_str(),
    true
  );

  Serial.println();
  Serial.println("JSON enviado:");

  Serial.println(json);

  Serial.println();
  Serial.println("Dados enviados para o HiveMQ.");
}

// ============================================================
// 18. SETUP
// ============================================================

void setup() {

  Serial.begin(115200);

  delay(1000);

  Serial.println();
  Serial.println("========================================");
  Serial.println("       ESTUFA ESP32 + HIVEMQ");
  Serial.println("========================================");

  // ==========================================================
  // RELE
  // ==========================================================

  pinMode(
    PINO_RELE,
    OUTPUT
  );

  // Comeca obrigatoriamente desligada

  digitalWrite(
    PINO_RELE,
    RELE_DESLIGADO
  );

  bombaLigada = false;

  // ==========================================================
  // SENSOR SOLO
  // ==========================================================

  pinMode(
    PINO_SOLO,
    INPUT
  );

  analogReadResolution(12);

  // ==========================================================
  // DHT
  // ==========================================================

  dht.begin();

  // ==========================================================
  // WIFI
  // ==========================================================

  conectarWiFi();

  // ==========================================================
  // TLS
  // ==========================================================
  //
  // Utilizado para simplificar o teste com HiveMQ Cloud.
  //

  wifiClient.setInsecure();

  // ==========================================================
  // MQTT
  // ==========================================================

  mqtt.setServer(
    MQTT_SERVIDOR,
    MQTT_PORTA
  );

  mqtt.setCallback(
    callback
  );

  mqtt.setBufferSize(1024);

  conectarMQTT();

  Serial.println();
  Serial.println("Sistema pronto.");
}

// ============================================================
// 19. LOOP
// ============================================================

void loop() {

  // ==========================================================
  // WIFI
  // ==========================================================

  if (
    WiFi.status() != WL_CONNECTED
  ) {

    conectarWiFi();
  }

  // ==========================================================
  // MQTT
  // ==========================================================

  if (
    WiFi.status() == WL_CONNECTED &&
    !mqtt.connected()
  ) {

    conectarMQTT();
  }

  mqtt.loop();

  // ==========================================================
  // ENVIO DOS SENSORES
  // ==========================================================

  if (
    millis() - ultimoEnvio
    >= INTERVALO_ENVIO
  ) {

    ultimoEnvio = millis();

    enviarDados();
  }

  delay(10);
}
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>

// =====================================================
// WIFI
// =====================================================

const char* WIFI_SSID = "NOME_DA_SUA_REDE";
const char* WIFI_SENHA = "SENHA_DA_SUA_REDE";

// =====================================================
// HIVEMQ CLOUD
// =====================================================

const char* MQTT_SERVIDOR =
  "d5010d2de7ff4182bd09c7fde4c243e5.s1.eu.hivemq.cloud";

const int MQTT_PORTA = 8883;

const char* MQTT_USUARIO = "thalita";
const char* MQTT_SENHA = "123456789";

// =====================================================
// TOPICOS MQTT
// =====================================================

const char* TOPICO_COMANDO =
  "estufa/bomba/comando";

const char* TOPICO_STATUS_BOMBA =
  "estufa/bomba/status";

const char* TOPICO_STATUS_ESP =
  "estufa/esp/status";

const char* TOPICO_RSSI =
  "estufa/esp/rssi";

const char* TOPICO_UPTIME =
  "estufa/esp/uptime";

// =====================================================
// RELE
// =====================================================

#define PINO_RELE 23

// Seu módulo normalmente é acionado em LOW
#define RELE_LIGADO LOW
#define RELE_DESLIGADO HIGH

bool bombaLigada = false;

// =====================================================
// MQTT
// =====================================================

WiFiClientSecure wifiClient;
PubSubClient mqtt(wifiClient);

// =====================================================
// TEMPORIZADOR
// =====================================================

unsigned long ultimoEnvio = 0;

const unsigned long INTERVALO_ENVIO = 10000;

// =====================================================
// LIGAR BOMBA
// =====================================================

void ligarBomba() {

  digitalWrite(PINO_RELE, RELE_LIGADO);

  bombaLigada = true;

  Serial.println();
  Serial.println("==========================");
  Serial.println("BOMBA LIGADA");
  Serial.println("==========================");

  mqtt.publish(
    TOPICO_STATUS_BOMBA,
    "LIGADA",
    true
  );
}

// =====================================================
// DESLIGAR BOMBA
// =====================================================

void desligarBomba() {

  digitalWrite(PINO_RELE, RELE_DESLIGADO);

  bombaLigada = false;

  Serial.println();
  Serial.println("==========================");
  Serial.println("BOMBA DESLIGADA");
  Serial.println("==========================");

  mqtt.publish(
    TOPICO_STATUS_BOMBA,
    "DESLIGADA",
    true
  );
}

// =====================================================
// RECEBE COMANDO MQTT
// =====================================================

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
  Serial.print("Topico recebido: ");
  Serial.println(topic);

  Serial.print("Mensagem: ");
  Serial.println(mensagem);

  // ===================================================
  // CONTROLE DA BOMBA
  // ===================================================

  if (String(topic) == TOPICO_COMANDO) {

    // Aceita vários comandos

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

      Serial.println("Comando desconhecido.");
    }
  }
}

// =====================================================
// CONECTAR WIFI
// =====================================================

void conectarWiFi() {

  Serial.println();
  Serial.print("Conectando ao WiFi: ");
  Serial.println(WIFI_SSID);

  WiFi.mode(WIFI_STA);

  WiFi.begin(
    WIFI_SSID,
    WIFI_SENHA
  );

  while (WiFi.status() != WL_CONNECTED) {

    delay(500);

    Serial.print(".");
  }

  Serial.println();
  Serial.println("==========================");
  Serial.println("WIFI CONECTADO");
  Serial.println("==========================");

  Serial.print("IP: ");
  Serial.println(WiFi.localIP());

  Serial.print("RSSI: ");
  Serial.println(WiFi.RSSI());
}

// =====================================================
// CONECTAR MQTT
// =====================================================

void conectarMQTT() {

  while (!mqtt.connected()) {

    Serial.println();
    Serial.println("Conectando ao HiveMQ...");

    // ID único da ESP32
    String clienteID = "ESP32-ESTUFA-";

    clienteID += String(
      (uint32_t)ESP.getEfuseMac(),
      HEX
    );

    // Last Will
    bool conectado = mqtt.connect(
      clienteID.c_str(),
      MQTT_USUARIO,
      MQTT_SENHA,
      TOPICO_STATUS_ESP,
      1,
      true,
      "OFFLINE"
    );

    if (conectado) {

      Serial.println("==========================");
      Serial.println("HIVEMQ CONECTADO");
      Serial.println("==========================");

      // ESP está online
      mqtt.publish(
        TOPICO_STATUS_ESP,
        "ONLINE",
        true
      );

      // Estado atual da bomba
      mqtt.publish(
        TOPICO_STATUS_BOMBA,
        bombaLigada ? "LIGADA" : "DESLIGADA",
        true
      );

      // Assina comando da bomba
      mqtt.subscribe(
        TOPICO_COMANDO
      );

      Serial.print("Inscrito em: ");
      Serial.println(TOPICO_COMANDO);
    }

    else {

      Serial.print("Erro MQTT: ");
      Serial.println(mqtt.state());

      Serial.println(
        "Tentando novamente em 5 segundos..."
      );

      delay(5000);
    }
  }
}

// =====================================================
// ENVIAR DADOS
// =====================================================

void enviarDados() {

  // ===================================================
  // RSSI
  // ===================================================

  String rssi =
    String(WiFi.RSSI());

  mqtt.publish(
    TOPICO_RSSI,
    rssi.c_str(),
    true
  );

  // ===================================================
  // UPTIME
  // ===================================================

  unsigned long segundos =
    millis() / 1000;

  String uptime =
    String(segundos);

  mqtt.publish(
    TOPICO_UPTIME,
    uptime.c_str(),
    true
  );

  // ===================================================
  // STATUS DA BOMBA
  // ===================================================

  mqtt.publish(
    TOPICO_STATUS_BOMBA,
    bombaLigada ? "LIGADA" : "DESLIGADA",
    true
  );

  Serial.println();
  Serial.println("----- DADOS MQTT -----");

  Serial.print("Bomba: ");

  if (bombaLigada) {

    Serial.println("LIGADA");

  } else {

    Serial.println("DESLIGADA");
  }

  Serial.print("RSSI: ");
  Serial.print(WiFi.RSSI());
  Serial.println(" dBm");

  Serial.print("Uptime: ");
  Serial.print(segundos);
  Serial.println(" segundos");

  Serial.println("----------------------");
}

// =====================================================
// SETUP
// =====================================================

void setup() {

  Serial.begin(115200);

  delay(1000);

  // ===================================================
  // RELE
  // ===================================================

  pinMode(
    PINO_RELE,
    OUTPUT
  );

  // Segurança:
  // inicia sempre com a bomba desligada

  digitalWrite(
    PINO_RELE,
    RELE_DESLIGADO
  );

  bombaLigada = false;

  // ===================================================
  // WIFI
  // ===================================================

  conectarWiFi();

  // ===================================================
  // TLS
  // ===================================================

  /*
     Para facilitar o primeiro teste,
     utiliza conexão TLS sem validar
     o certificado do servidor.
  */

  wifiClient.setInsecure();

  // ===================================================
  // MQTT
  // ===================================================

  mqtt.setServer(
    MQTT_SERVIDOR,
    MQTT_PORTA
  );

  mqtt.setCallback(
    callback
  );

  mqtt.setBufferSize(512);

  conectarMQTT();
}

// =====================================================
// LOOP
// =====================================================

void loop() {

  // ===================================================
  // VERIFICA WIFI
  // ===================================================

  if (
    WiFi.status() != WL_CONNECTED
  ) {

    conectarWiFi();
  }

  // ===================================================
  // VERIFICA MQTT
  // ===================================================

  if (
    !mqtt.connected()
  ) {

    conectarMQTT();
  }

  mqtt.loop();

  // ===================================================
  // ENVIA DADOS A CADA 10 SEGUNDOS
  // ===================================================

  if (
    millis() - ultimoEnvio
    >= INTERVALO_ENVIO
  ) {

    ultimoEnvio = millis();

    enviarDados();
  }
}
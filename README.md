# Sistema de Irrigação com ESP32, HiveMQ e Bomba 12 V

Este projeto utiliza uma **ESP32** para controlar remotamente uma **bomba de água 12 V** através de um **módulo relé 5 V**. A comunicação é realizada pela internet usando o protocolo **MQTT** e um broker **HiveMQ Cloud**.

O sistema permite ligar e desligar a bomba pelo celular, acompanhar o estado da bomba, verificar se a ESP32 está online e receber informações básicas como intensidade do sinal Wi-Fi e tempo de funcionamento da ESP32.

---

## Visão geral do sistema

```text
                    INTERNET
                       │
                       │ MQTT
                       ▼
                ┌─────────────┐
                │ HiveMQ Cloud│
                └──────┬──────┘
                       │
                    Wi-Fi
                       │
                       ▼
                  ┌─────────┐
                  │  ESP32  │
                  └────┬────┘
                       │ GPIO 23
                       ▼
                 ┌───────────┐
                 │ Relé 5 V  │
                 └─────┬─────┘
                       │
                    COM / NO
                       │
                       ▼
                 ┌───────────┐
                 │ Bomba 12V │
                 └───────────┘
```

---

# 1. Componentes

| Quantidade | Componente |
|---:|---|
| 1 | ESP32 DevKit |
| 1 | Módulo relé 1 canal 5 V |
| 1 | Bomba de água 12 V |
| 1 | Fonte ou bateria 12 V |
| 1 | Conversor DC-DC 12 V → 5 V, se necessário |
| 1 | Diodo 1N4007 recomendado |
| — | Jumpers e fios |
| — | Rede Wi-Fi com acesso à internet |
| — | Conta HiveMQ Cloud |

Se o sistema utilizar alimentação solar, também podem ser utilizados painel solar, controlador de carga e bateria de 12 V.

---

# 2. Pinagem utilizada na ESP32

| ESP32 | Função |
|---|---|
| `GPIO 23` | Controle do relé |
| `VIN / 5V` | Alimentação do módulo relé |
| `GND` | Terra do módulo relé |

Neste projeto não são necessários display ou sensores.

---

# 3. Ligação entre ESP32 e relé

O módulo relé possui normalmente três pinos de controle:

| Relé | ESP32 |
|---|---|
| `IN` | GPIO 23 |
| `VCC` | 5V / VIN |
| `GND` | GND |

Ligação:

```text
ESP32                     MÓDULO RELÉ
┌─────────────┐           ┌──────────────┐
│             │           │              │
│ GPIO 23 ───────────────► IN            │
│             │           │              │
│ 5V/VIN ─────────────────► VCC           │
│             │           │              │
│ GND ────────────────────► GND           │
│             │           │              │
└─────────────┘           └──────────────┘
```

O pino `GPIO 23` não fornece energia para a bomba. Ele envia apenas o sinal de controle para o módulo relé.

---

# 4. Bornes do relé

O módulo utilizado possui três contatos para a carga:

| Terminal | Significado | Uso neste projeto |
|---|---|---|
| `NC` | Normally Closed | Não utilizado |
| `COM` | Common | Recebe +12 V |
| `NO` | Normally Open | Vai para o positivo da bomba |

A bomba deve permanecer desligada quando o sistema estiver em repouso. Por isso são utilizados os contatos **COM e NO**.

```text
                 RELÉ

             ┌─────────┐
Não usar ────┤ NC      │
             │         │
+12 V ───────┤ COM     │
             │         │
+BOMBA ◄─────┤ NO      │
             └─────────┘
```

---

# 5. Ligação da bomba 12 V

A alimentação de 12 V não deve passar pela ESP32.

A ligação correta é:

| Origem | Destino |
|---|---|
| +12 V da fonte/bateria | COM do relé |
| NO do relé | Positivo da bomba |
| Negativo da bomba | Negativo da fonte/bateria |
| NC | Não utilizar |

Diagrama:

```text
+12 V DA FONTE
      │
      │
      ▼
     COM
      │
   ┌───────┐
   │ RELÉ  │
   └───────┘
      │
      NO
      │
      ▼
   + BOMBA
     12 V
   - BOMBA
      │
      │
      ▼
-12 V DA FONTE
```

Quando o relé estiver desligado:

```text
COM    NO

 ●     ●

Circuito aberto
Bomba desligada
```

Quando o relé for acionado:

```text
COM────NO

 ●─────●

Circuito fechado
Bomba ligada
```

---

# 6. Diodo de proteção da bomba

É recomendado colocar um **1N4007 em paralelo com a bomba**, principalmente quando ela utiliza motor DC.

```text
                 + BOMBA
                    │
        ┌───────────┴──────────┐
        │                      │
        │      1N4007          │
        │     ──|<|──          │
        │                      │
        └───────────┬──────────┘
                    │
                 - BOMBA
```

A **faixa do diodo** deve ficar no lado positivo da bomba.

```text
+BOMBA ───────|<|────── -BOMBA
              ↑
             faixa
```

Mais precisamente:

```text
Cátodo/faixa → positivo da bomba
Ânodo        → negativo da bomba
```

O diodo ajuda a reduzir picos de tensão produzidos pelo motor.

---

# 7. Alimentação com conversor DC-DC

Caso todo o projeto seja alimentado por uma bateria de 12 V, não conecte os 12 V diretamente no pino 5 V da ESP32.

Use um conversor **buck 12 V → 5 V**.

```text
BATERIA 12 V
   │
   ├──── +12 V ───────────────► circuito da bomba
   │
   └──── +12 V
          │
          ▼
      ┌───────────┐
      │ BUCK      │
      │ 12V → 5V  │
      └─────┬─────┘
            │
           5 V
            │
            ├────────► VIN/5V ESP32
            │
            └────────► VCC do relé
```

Tabela:

| Conversor Buck | Ligação |
|---|---|
| `IN+` | +12 V |
| `IN-` | negativo da bateria |
| `OUT+` | 5 V |
| `OUT-` | GND |
| ESP32 `VIN/5V` | OUT+ |
| ESP32 `GND` | OUT- |
| Relé `VCC` | OUT+ |
| Relé `GND` | OUT- |

Antes de conectar a ESP32, ajuste o conversor e confirme com um multímetro que a saída está aproximadamente em **5 V**.

---

# 8. Sistema completo

```text
                       HIVE MQ CLOUD
                            │
                            │ Internet
                            │
                           Wi-Fi
                            │
                            ▼
                     ┌─────────────┐
                     │    ESP32    │
                     │             │
                     │ GPIO23      │
                     │ 5V          │
                     │ GND         │
                     └──┬──┬──┬───┘
                        │  │  │
              GPIO23 ───┘  │  └──────── GND
                           │
                          5 V
                           │
                           ▼
                     ┌─────────────┐
                     │ RELÉ 5 V    │
                     │             │
              GPIO23 ► IN          │
                  5V ► VCC         │
                 GND ► GND         │
                     │             │
               +12V ► COM         │
                     │             │
              BOMBA ◄ NO          │
                     │             │
                     │ NC — NÃO USAR
                     └─────────────┘

+12 V ─────────────► COM
                       │
                       │ Relé acionado
                       ▼
NO ────────────────► + BOMBA

- BOMBA ───────────► -12 V
```

---

# 9. Comunicação MQTT

O projeto utiliza o protocolo MQTT.

Broker utilizado:

```text
d5010d2de7ff4182bd09c7fde4c243e5.s1.eu.hivemq.cloud
```

Porta:

```text
8883
```

Segurança:

```text
TLS/SSL
```

Usuário MQTT:

```text
thalita
```

Por segurança, **não coloque a senha real no README caso o projeto seja enviado ao GitHub**.

No código utilize:

```cpp
const char* MQTT_USUARIO = "thalita";
const char* MQTT_SENHA = "SUA_SENHA_MQTT";
```

O mesmo vale para o Wi-Fi:

```cpp
const char* WIFI_SSID = "SUA_REDE_WIFI";
const char* WIFI_SENHA = "SUA_SENHA_WIFI";
```

---

# 10. Tópicos MQTT

O projeto utiliza os seguintes tópicos:

| Tópico | Função |
|---|---|
| `estufa/bomba/comando` | Recebe comando para ligar/desligar |
| `estufa/bomba/status` | Informa estado atual da bomba |
| `estufa/esp/status` | Informa se a ESP32 está online |
| `estufa/esp/rssi` | Intensidade do Wi-Fi |
| `estufa/esp/uptime` | Tempo desde que a ESP32 iniciou |

---

# 11. Comando para ligar a bomba

Publicar no tópico:

```text
estufa/bomba/comando
```

Mensagem:

```text
LIGAR
```

Também são aceitos:

```text
ON
```

ou:

```text
1
```

A ESP32 aciona:

```text
GPIO23 → Relé → Bomba
```

E publica:

```text
estufa/bomba/status
```

com:

```text
LIGADA
```

---

# 12. Comando para desligar

Publicar:

```text
estufa/bomba/comando
```

Mensagem:

```text
DESLIGAR
```

Também podem ser utilizados:

```text
OFF
```

ou:

```text
0
```

Depois disso, a ESP32 publica:

```text
estufa/bomba/status
```

com:

```text
DESLIGADA
```

---

# 13. Estado da ESP32

Ao conectar ao HiveMQ, a ESP32 publica:

```text
estufa/esp/status
```

Mensagem:

```text
ONLINE
```

Se a ESP32 perder inesperadamente a conexão MQTT, o recurso **Last Will and Testament (LWT)** do MQTT permite que o broker publique:

```text
OFFLINE
```

Assim é possível identificar pelo celular se o equipamento está conectado.

---

# 14. RSSI

O tópico:

```text
estufa/esp/rssi
```

informa a intensidade do sinal Wi-Fi.

Exemplo:

```text
-55
```

O valor é fornecido em `dBm`.

De maneira aproximada:

| RSSI | Qualidade |
|---:|---|
| -30 dBm | Excelente |
| -50 dBm | Muito boa |
| -60 dBm | Boa |
| -70 dBm | Razoável |
| -80 dBm | Fraca |

---

# 15. Uptime

A ESP32 envia:

```text
estufa/esp/uptime
```

Exemplo:

```text
3600
```

Isso significa que a ESP32 está funcionando há:

```text
3600 segundos = 1 hora
```

---

# 16. Biblioteca necessária

No Arduino IDE, instale:

```text
PubSubClient
```

Autor:

```text
Nick O'Leary
```

As bibliotecas:

```cpp
#include <WiFi.h>
#include <WiFiClientSecure.h>
```

fazem parte do pacote da ESP32.

---

# 17. Configuração da Arduino IDE

Selecione uma placa ESP32 compatível, por exemplo:

```text
Tools
→ Board
→ ESP32 Arduino
→ ESP32 Dev Module
```

Depois selecione a porta:

```text
Tools
→ Port
→ COM...
```

Se aparecer:

```text
WiFi.h: No such file or directory
```

normalmente significa que uma placa Arduino Uno ou outra placa incompatível está selecionada.

Certifique-se de ter instalado o pacote:

```text
esp32 by Espressif Systems
```

no **Boards Manager** da Arduino IDE.

---

# 18. Funcionamento do relé

O código foi preparado considerando um módulo de relé **ativo em nível LOW**:

```cpp
#define RELE_LIGADO LOW
#define RELE_DESLIGADO HIGH
```

Isso significa:

```text
GPIO 23 = LOW
→ Relé acionado
→ COM conectado ao NO
→ Bomba ligada
```

e:

```text
GPIO 23 = HIGH
→ Relé desacionado
→ COM separado do NO
→ Bomba desligada
```

Se o seu módulo funcionar ao contrário, altere para:

```cpp
#define RELE_LIGADO HIGH
#define RELE_DESLIGADO LOW
```

---

# 19. Estado inicial de segurança

Sempre que a ESP32 iniciar, o programa começa com:

```cpp
digitalWrite(PINO_RELE, RELE_DESLIGADO);
```

Isso faz com que a bomba permaneça **desligada até receber um comando**.

É uma medida importante para evitar que a bomba ligue automaticamente após uma queda de energia ou reinicialização.

---

# 20. Controle pelo celular

Pode ser utilizado qualquer aplicativo MQTT compatível com TLS.

Configure a conexão com:

```text
Host:
d5010d2de7ff4182bd09c7fde4c243e5.s1.eu.hivemq.cloud

Porta:
8883

TLS/SSL:
Ativado

Usuário:
thalita
```

Use a senha configurada no HiveMQ Cloud.

Para criar um botão **LIGAR**:

```text
Topic:
estufa/bomba/comando

Payload:
LIGAR
```

Para criar um botão **DESLIGAR**:

```text
Topic:
estufa/bomba/comando

Payload:
DESLIGAR
```

Para exibir o estado da bomba, assine:

```text
estufa/bomba/status
```

---

# 21. Fluxo de funcionamento

```text
CELULAR
   │
   │ publica "LIGAR"
   ▼
HiveMQ Cloud
   │
   ▼
ESP32
   │
GPIO 23
   │
   ▼
RELÉ
   │
COM conecta ao NO
   │
   ▼
BOMBA 12 V LIGA
```

Quando o usuário toca em desligar:

```text
CELULAR
   │
   │ publica "DESLIGAR"
   ▼
HiveMQ
   │
   ▼
ESP32
   │
GPIO 23
   │
   ▼
RELÉ DESACIONA
   │
COM desconecta do NO
   │
   ▼
BOMBA DESLIGA
```

---

# 22. Cuidados importantes

**Nunca conecte a bomba diretamente na ESP32.**

A bomba deve receber energia diretamente da alimentação de **12 V**, sendo interrompida pelo contato do relé.

A saída GPIO da ESP32 serve apenas para enviar o sinal de comando.

Também não aplique:

```text
12 V
```

nos pinos:

```text
GPIO
3V3
5V
```

da ESP32.

Se utilizar conversor buck, confirme a tensão de saída antes de conectar a placa.

Outro ponto importante é que o relé mostrado utiliza bobina de **5 V**. Alguns módulos de relé 5 V não reconhecem perfeitamente os **3,3 V** do GPIO da ESP32 no pino `IN`. Se o módulo não acionar de forma confiável, utilize um módulo compatível com lógica de 3,3 V ou um transistor/driver entre a ESP32 e o relé.

---

# 23. Resumo final das ligações

| De | Para |
|---|---|
| ESP32 GPIO23 | IN do relé |
| 5V/VIN ESP32 ou buck 5V | VCC do relé |
| ESP32 GND | GND do relé |
| +12 V | COM |
| NO | + da bomba |
| - da bomba | -12 V |
| NC | Não conectar |
| Diodo 1N4007 cátodo/faixa | + da bomba |
| Diodo 1N4007 ânodo | - da bomba |

O caminho da potência é:

```text
+12V
 ↓
COM
 ↓
NO
 ↓
+BOMBA
 ↓
BOMBA
 ↓
-BOMBA
 ↓
-12V
```

O caminho de controle é:

```text
CELULAR
   ↓
HIVEMQ
   ↓
WI-FI
   ↓
ESP32
   ↓
GPIO 23
   ↓
IN DO RELÉ
   ↓
COM/NO
   ↓
BOMBA
```

---

## Resultado

Ao final, o projeto permite controlar uma bomba de irrigação de **12 V pela internet utilizando uma ESP32 e MQTT**.

A ESP32 permanece conectada ao HiveMQ e aguarda comandos. Ao receber `LIGAR`, o relé é acionado e fornece alimentação para a bomba. Ao receber `DESLIGAR`, o relé interrompe o circuito.

Além do controle remoto, o sistema fornece informações de:

```text
Estado da bomba
Estado online/offline da ESP32
Intensidade do Wi-Fi
Tempo de funcionamento
```

Isso cria uma base pronta para futuramente adicionar sensor de umidade do solo, temperatura, automação da irrigação, página web, aplicativo Android ou monitoramento completo da estufa.

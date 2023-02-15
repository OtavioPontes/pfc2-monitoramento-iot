#include <WiFi.h> //Lib WiFi
#include <SimpleDHT.h> //Lib DHT
#include <ArduinoJson.h> //Lib para a manipulação de Json
#include <PubSubClient.h> //Lib MQTT
#include <ESPDateTime.h>

#define DHTPIN 23 //Pino onde o DHT está ligado
#define INTERVAL 5000 //Intervalo entre cada leitura do sensor

//SSID e senha da rede WiFi onde o esp32 irá se conectar
#define SSID "AP1301_2G"
#define PASSWORD "saturno6"

#define TOKEN "BBFF-Ot80gw3zfXheHSO9LqEkmtY6E9JWw3" 
#define VARIABLE_LABEL_TEMPERATURE "temperatura" //Label referente a variável de temperatura
#define VARIABLE_LABEL_HUMIDITY "umidade" //Label referente a variável de umidade
#define DEVICE_ID "62f854c1c15bf406acfd68f6" //ID do dispositivo
#define SERVER "192.168.0.105"

//Porta padrão
#define PORT 1883


//Tópico aonde serão feitos os publish
#define TOPIC "sensordata"
#define NTP_SERVER "pool.ntp.org"

WiFiClient espClient;
PubSubClient client(espClient);

char mensagem[30];

//Objeto que realiza a leitura da umidade e temperatura
SimpleDHT11 dht;

//Variáveis que vão guardar o valor da temperatura e umidade
float temperature, humidity;

//Marca quando foi feita a última leitura
uint32_t lastTimeRead = 0;

void setup() {
  Serial.begin(115200);
  configTime(0, 3600, NTP_SERVER);
  //Inicializa a conexão com a rede WiFi
  setupWiFi();
  setupDateTime();
}

char* getCurrentTime() {
  struct tm timeinfo;
  getLocalTime(&timeinfo);
  Serial.println("Corno");
  char* isoTime;
  strftime(isoTime, 50, "%Y-%B-%dT%H:%M:%S", &timeinfo);

  return isoTime;
}

void loop() {
  struct tm timeinfo;
  //Tempo em millissegundos desde o boot do esp
  unsigned long now = millis();

  //Se passou o intervalo desde a última leitura
  if (now - lastTimeRead > INTERVAL) {
    //Faz a leitura do sensor
    readSensor();

    //Marca quando ocorreu a última leitura
    lastTimeRead = now;
  }

  //Se o esp foi desconectado do broker, tentamos reconectar
  if (!client.connected()) {
    reconnect();
  }

  if (sendValues(temperature, humidity)) {
    Serial.println("Dados enviados com sucesso...");
  }

  delay(2500);

}

void reconnect() {
  //Conexao ao broker MQTT
  client.setServer(SERVER, PORT);
  while (!client.connected())
  {
    Serial.println("Conectando ao broker MQTT...");
    if (client.connect(DEVICE_ID, TOKEN, ""))
    {
      Serial.println("Conectado ao broker!");
    }
    else
    {
      Serial.print("Falha na conexao ao broker - Estado: ");
      Serial.print(client.state());
      delay(2000);
    }
  }
}

//Realiza a leitura da temperatura e umidade
void readSensor() {
  float t, h;
  //Coloca o valor lido da temperatura em t e da umidade em h
  int status = dht.read2(DHTPIN, &t, &h, NULL);

  //Se a leitura foi bem sucedida
  if (status == SimpleDHTErrSuccess) {
    //Os valores foram lidos corretamente, então é seguro colocar nas variáveis
    temperature = t;
    humidity = h;
  }
  Serial.println(temperature);
  Serial.println(humidity);
}


//Conexão com a rede WiFi
void setupWiFi() {

  WiFi.disconnect();
  WiFi.mode(WIFI_STA);

  //Tenta conectar à rede que possui este SSID e senha
  WiFi.begin(SSID, PASSWORD);
  Serial.println("");

  //Enquanto não estiver conectado a rede WiFi
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  //Se chegou aqui está conectado
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(SSID);

  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

bool sendValues(float temperature, float humidity)
{
  char json[250];

  //Atribui para a cadeia de caracteres "json" os valores referentes a temperatura e os envia para a variável do ubidots correspondente
  //sprintf(json,  "{\"%s\":{\"value\":%02.02f, \"context\":{\"temperature\":%02.02f, \"humidity\":%02.02f}}}", VARIABLE_LABEL_TEMPERATURE, temperature, temperature, humidity);

  sprintf(json, "{\"date\":\"%s\",\"temperature\": %02.02f,\"humidity\": %02.02f,\"device_id\":\"%s\"}", DateTime.toISOString().c_str(), temperature, humidity, DEVICE_ID);


  if (!client.publish(TOPIC, json)) {
    return false;
    
  }

  //Se tudo der certo retorna true
  return true;
}

void setupDateTime() {
  // setup this after wifi connected
  // you can use custom timeZone,server and timeout
  // DateTime.setTimeZone(-4);
  //   DateTime.setServer("asia.pool.ntp.org");
  //   DateTime.begin(15 * 1000);
  DateTime.setServer("time.pool.aliyun.com");
  DateTime.setTimeZone("GMT-3");
  DateTime.begin();
  if (!DateTime.isTimeValid()) {
    Serial.println("Failed to get time from server.");
  } else {
    Serial.printf("Date Now is %s\n", DateTime.toISOString());
    Serial.printf("Timestamp is %ld\n", DateTime.now());
  }
}
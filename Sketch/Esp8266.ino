#include <OneWire.h>
#include <ESP8266WiFi.h>
#include <DallasTemperature.h>
#include <PubSubClient.h>
#include "secrets.h"

#define temperature_topic "sensor/temperature"
#define AVAILABILITY_TOPIC_LEFT "sensor/avaliability"

WiFiClient espClient;
PubSubClient client(espClient);

  
const int oneWireBus = 4; // GPIO where the DS18B20 is connected to (D2)
const int relay = 5;      // GPIO where relay is connected (D1)

OneWire oneWire(oneWireBus);
DallasTemperature sensors(&oneWire); // Passing oneWire reference to Dallas Temperature sensor 

void setup() {
  Serial.begin(9600);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  connect();
  sensors.begin();
  pinMode(relay, OUTPUT);
  
}

void setup_wifi() {
  delay(10);
  // Connecting to WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(wifi_ssid);

  WiFi.begin(wifi_ssid, wifi_password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void connect() {
  
  while (!client.connected()) {   // Loop until connected/reconnected
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    //boolean connect (clientID, [username, password], [willTopic, willQoS, willRetain, willMessage], [cleanSession])
    if (client.connect("ESP8266Client", mqtt_user, mqtt_password, AVAILABILITY_TOPIC_LEFT,1,true,"offline")) {
      Serial.println("connected");
      //boolean publish (topic, payload, [length], [retained])
      client.publish(AVAILABILITY_TOPIC_LEFT, "online");         // Once connected, publish online to the availability topic
+     client.publish(AVAILABILITY_TOPIC_LEFT, "online", true);
      client.subscribe("esp/test");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void toggleRelay() {
  int contact = digitalRead(relay);
  if (contact == HIGH) {
    digitalWrite(relay,LOW);
  }
  if (contact == LOW) {
    digitalWrite(relay,HIGH);
  }
}

void relayOn() {
  digitalWrite(relay,HIGH);
}
void relayOff() {
  digitalWrite(relay,LOW);
}

bool checkBound(float newValue, float prevValue, float maxDiff) {  //Is newValue +- maxDiff bigger or smaller than previous
  return !isnan(newValue) &&
         (newValue < prevValue - maxDiff || newValue > prevValue + maxDiff);
}

long lastMsg = 0;
float temp = 0.0;
float diff = 0.1;
float desiredTemp = 26;


void callback(char* topic, byte* payload, unsigned int length) { //Mqtt subscriptions ends up here
  
  Serial.print("Message arrived in topic: ");
  Serial.println(topic);
  char pL[length];
  Serial.print("Message:");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
    pL[i]= (char)payload[i];
    
  }

  Serial.println();
  Serial.println("-----------------------");
  float num = atof(pL);
  Serial.println("Converted to float");
  Serial.println(num);
  desiredTemp=num;
  toggleRelay();
 
}

void loop() {
  if (!client.connected()) {
    connect();
  }
  client.loop();
  long now = millis();
  if (now - lastMsg > 30000) {
    lastMsg = now;
    sensors.requestTemperatures();
    float newTemp = sensors.getTempCByIndex(0);

    if (checkBound(newTemp, temp, diff)) {
      temp = newTemp;
      Serial.print("New temperature:");
      Serial.println(String(temp).c_str());
      client.publish(temperature_topic, String(temp).c_str(), true);
      Serial.print("Destemperature: ");
      Serial.println(desiredTemp);
    }
  }
  if (temp < desiredTemp) {
    relayOn();
  } else {
    relayOff();
  }

}
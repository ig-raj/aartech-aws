#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <DHT.h>
#include<time.h>

// WiFi credentials
const char* ssid = "your wifi ssid";
const char* password = "your password";

// AWS IoT Core Credentials
const char* AWS_IOT_ENDPOINT = "your endpoint"; // From AWS IoT Core

// AWS IoT Certificates (Paste contents between the quotes)
const char* AWS_CERT_CA = R"EOF(
-----BEGIN CERTIFICATE-----

-----END CERTIFICATE-----
)EOF";

const char* AWS_CERT_CRT = R"EOF(
-----BEGIN CERTIFICATE-----

)EOF";

const char* AWS_CERT_PRIVATE = R"EOF(
-----BEGIN RSA PRIVATE KEY-----

-----END RSA PRIVATE KEY-----

)EOF";
// DHT11 Setup
#define DHTPIN 2
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// WiFi + MQTT clients
WiFiClientSecure secureClient;
PubSubClient client(secureClient);

// Time sync (needed for TLS)
void syncTime() {
  configTime(0, 0, "pool.ntp.org", "time.nist.gov");
  Serial.print("Waiting for NTP time sync");
  time_t now = time(nullptr);
  while (now < 8 * 3600 * 2) {
    delay(500);
    Serial.print(".");
    now = time(nullptr);
  }
  Serial.println(" time synchronized!");
}

// MQTT callback (optional for this case)
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (unsigned int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
}

// Connect to WiFi
void connectToWiFi() {
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println(" connected!");
}
const int AWS_IOT_PORT = 8883;
const char* topic = "esp8266";

// Connect to AWS IoT
void connectToAWS() {
  BearSSL::X509List ca(AWS_CERT_CA);
  BearSSL::X509List cert(AWS_CERT_CRT);
  BearSSL::PrivateKey key(AWS_CERT_PRIVATE);
  
  secureClient.setTrustAnchors(&ca);
  secureClient.setClientRSACert(&cert, &key);
  
  client.setServer(AWS_IOT_ENDPOINT, AWS_IOT_PORT);
  
  Serial.print("Connecting to AWS IoT");
  while (!client.connected()) {
    if (client.connect("ESP8266_Client")) {
      Serial.println(" connected!");
    } else {
      Serial.print(".");
      delay(1000);
    }
  }
}

// Reconnect if MQTT drops
void reconnectMQTT() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect("ESP8266_Client")) {
      Serial.println(" connected.");
      // Optional: client.subscribe("some/topic");
    } else {
      Serial.print(" failed, rc=");
      Serial.print(client.state());
      Serial.println(" retrying in 5 sec");
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  connectToWiFi();
  syncTime();
  dht.begin();
  client.setCallback(mqttCallback);
  connectToAWS();
}

void loop() {
  if (!client.connected()) {
    reconnectMQTT();
  }
  client.loop();

  // Read temperature & humidity
  float temp = dht.readTemperature();
  float hum = dht.readHumidity();

  if (isnan(temp) || isnan(hum)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }

  // Format and publish payload
  String payload = "{\"temperature\": " + String(temp) + ", \"humidity\": " + String(hum) + "}";
  if (client.publish(topic, payload.c_str())) {
    Serial.println("Published: " + payload);
  } else {
    Serial.println("Publish failed");
  }

  delay(5000); // Wait 5 seconds before sending again
}
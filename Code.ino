#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include <MPU6050.h>
#include "DHT.h"

#define DHTPIN 14
#define DHTTYPE DHT22
#define IR_SENSOR_PIN 27
#define BUZZER_PIN 25
#define LED_PIN 26
#define MQ2_PIN 36  // VP analog input

#define I2C_SDA 21
#define I2C_SCL 22

const char* ssid = "Divit";
const char* password = "Divit7002";

WebServer server(80);
DHT dht(DHTPIN, DHTTYPE);
MPU6050 mpu;

String webPage;
float temperature = 0;
float humidity = 0;
int gasValue = 0;
int helmetWorn = 0;
float gForce = 0.0;

void setup() {
  Serial.begin(115200);
  dht.begin();
  pinMode(IR_SENSOR_PIN, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);
  Wire.begin(I2C_SDA, I2C_SCL);
  mpu.initialize();

  WiFi.begin(ssid, password);
  Serial.print("Connecting");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500); Serial.print(".");
  }
  Serial.println("\nConnected: " + WiFi.localIP().toString());

  // Route Handlers
  server.on("/", handleRoot);
  server.on("/data", HTTP_GET, handleSensorData);
  server.begin();
}

void loop() {
  readSensors();
  handleSafety();
  server.handleClient();
}

void readSensors() {
  temperature = dht.readTemperature();
  humidity = dht.readHumidity();
  gasValue = analogRead(MQ2_PIN);
  helmetWorn = digitalRead(IR_SENSOR_PIN);

  int16_t ax, ay, az;
  mpu.getAcceleration(&ax, &ay, &az);
  gForce = sqrt(ax * ax + ay * ay + az * az) / 16384.0;
}

void handleSafety() {
  if (gForce > 2.5 || helmetWorn == HIGH) {
    digitalWrite(LED_PIN, HIGH);
    digitalWrite(BUZZER_PIN, HIGH);
  } else {
    digitalWrite(LED_PIN, LOW);
    digitalWrite(BUZZER_PIN, LOW);
  }
}

void handleSensorData() {
  String json = "{";
  json += "\"temperature\":" + String(temperature) + ",";
  json += "\"humidity\":" + String(humidity) + ",";
  json += "\"gas\":" + String(gasValue) + ",";
  json += "\"helmet\":" + String(helmetWorn) + ",";
  json += "\"gforce\":" + String(gForce);
  json += "}";
  server.send(200, "application/json", json);
}

void handleRoot() {
  server.send(200, "text/html", webpageHTML);
}

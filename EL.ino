#include <WiFi.h>
#include <Wire.h>
#include <MPU6050.h>
#include <DHT.h>
#include <WebServer.h>
#include <SPIFFS.h>

#define DHTPIN 14
#define DHTTYPE DHT22
#define IR_SENSOR_PIN 27
#define BUZZER_PIN 25
#define LED_PIN 26
#define MQ2_PIN 36

MPU6050 mpu;
DHT dht(DHTPIN, DHTTYPE);
WebServer server(80);

float gForce = 0.0;
float temperature = 0.0;
float humidity = 0.0;
int gas = 0;
int helmet = 0;
bool hazard = false;

void handleRoot() {
  File file = SPIFFS.open("/index.html", "r");
  if (!file) {
    server.send(500, "text/plain", "File not found");
    return;
  }
  server.streamFile(file, "text/html");
  file.close();
}

void handleData() {
  String json = "{";
  json += "\"temperature\":" + String(temperature) + ",";
  json += "\"humidity\":" + String(humidity) + ",";
  json += "\"gas\":" + String(gas) + ",";
  json += "\"gforce\":" + String(gForce, 2) + ",";
  json += "\"helmet\":" + String(helmet) + ",";
  json += "\"hazard\":\"" + String(hazard ? "⚠️ HAZARD DETECTED!" : "✅ Safe") + "\"";
  json += "}";
  server.send(200, "application/json", json);
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  pinMode(IR_SENSOR_PIN, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);
  digitalWrite(LED_PIN, LOW);

  WiFi.softAP("SmartHelmet_AP", "helmet123");
  Serial.println("✅ Access Point Started");
  Serial.print("IP Address: ");
  Serial.println(WiFi.softAPIP()); 

  if (!SPIFFS.begin(true)) {
    Serial.println("❌ Failed to mount SPIFFS");
    return;
  }

  Wire.begin(19, 18); 
  mpu.initialize();
  dht.begin();

  server.on("/", handleRoot);
  server.on("/data", handleData);
  server.begin();
}

void loop() {
  server.handleClient();

  int16_t ax, ay, az;
  mpu.getAcceleration(&ax, &ay, &az);
  gForce = sqrt(ax * ax + ay * ay + az * az) / 16384.0;

  temperature = dht.readTemperature();
  humidity = dht.readHumidity();
  gas = analogRead(MQ2_PIN);
  helmet = digitalRead(IR_SENSOR_PIN);

  hazard = (gForce > 2.5 || gForce < 0.2 || gas > 600 || temperature > 40 || helmet == HIGH);

  digitalWrite(BUZZER_PIN, hazard ? HIGH : LOW);
  digitalWrite(LED_PIN, hazard ? HIGH : LOW);

  Serial.printf("G-Force: %.2f g | Temp: %.2f °C | Hum: %.2f %% | Gas: %d | Helmet: %s | Status: %s\n",
                gForce, temperature, humidity, gas,
                helmet == 0 ? "WORN" : "NOT WORN",
                hazard ? "⚠️ HAZARD DETECTED!" : "✅ Safe");

  delay(1000);
}

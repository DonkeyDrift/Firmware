#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include <MPU6050.h>

const char* ssid = "DriftJudge_AP";
const char* password = "12345678";

WebServer server(80);

MPU6050 mpu;
int16_t ax, ay, az;
int16_t gx, gy, gz;

float throttle = 0;
float rpm = 0;
unsigned long lastTime = 0;
unsigned long sampleInterval = 20;

float gyroZHistory[10];
int gyroZIndex = 0;

float calculateGyroZRate() {
  float sum = 0;
  for (int i = 0; i < 10; i++) {
    sum += gyroZHistory[i];
  }
  return sum / 10.0;
}

void pushGyroZ(float val) {
  gyroZHistory[gyroZIndex] = val;
  gyroZIndex = (gyroZIndex + 1) % 10;
}

void handleRoot() {
  String html = "<!DOCTYPE HTML><html><head><meta charset='UTF-8'>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<title>Drift Judge</title></head><body>";
  html += "<h1>漂移裁判系统</h1>";
  html += "<p><a href='/index.html'>打开评分界面</a></p>";
  html += "<p><a href='/sse'>SSE数据流</a></p>";
  html += "</body></html>";
  server.send(200, "text/html", html);
}

void handleSSE() {
  WiFiClient client = server.client();
  
  server.sendHeader("Cache-Control", "no-cache");
  server.sendHeader("Content-Type", "text/event-stream");
  server.sendHeader("Connection", "keep-alive");
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "text/event-stream", "");
  
  unsigned long lastSend = 0;
  
  while (client.connected()) {
    unsigned long now = millis();
    if (now - lastSend >= sampleInterval) {
      lastSend = now;
      
      mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);
      
      float gyroZ = gz / 131.0;
      pushGyroZ(gyroZ);
      float gyroZRate = calculateGyroZRate();
      
      float accelX = ax / 16384.0;
      float accelY = ay / 16384.0;
      float accelZ = az / 16384.0;
      float gyroX = gx / 131.0;
      float gyroY = gy / 131.0;
      
      float speed = rpm * 0.01;
      
      String data = "data: {";
      data += "\"ts\":" + String(now) + ",";
      data += "\"throttle\":" + String(throttle, 2) + ",";
      data += "\"rpm\":" + String(rpm, 1) + ",";
      data += "\"speed\":" + String(speed, 2) + ",";
      data += "\"accel_x\":" + String(accelX, 3) + ",";
      data += "\"accel_y\":" + String(accelY, 3) + ",";
      data += "\"accel_z\":" + String(accelZ, 3) + ",";
      data += "\"gyro_x\":" + String(gyroX, 3) + ",";
      data += "\"gyro_y\":" + String(gyroY, 3) + ",";
      data += "\"gyro_z\":" + String(gyroZ, 3) + ",";
      data += "\"gyro_z_rate\":" + String(gyroZRate, 3);
      data += "}\n\n";
      
      client.print(data);
      client.flush();
    }
    
    delay(5);
  }
}

void handleIndex() {
  File file = SPIFFS.open("/index.html", "r");
  if (!file) {
    server.send(404, "text/plain", "File not found");
    return;
  }
  server.streamFile(file, "text/html");
  file.close();
}

void handleAppJS() {
  File file = SPIFFS.open("/app.js", "r");
  if (!file) {
    server.send(404, "text/plain", "File not found");
    return;
  }
  server.streamFile(file, "application/javascript");
  file.close();
}

void setup() {
  Serial.begin(115200);
  delay(100);
  
  SPIFFS.begin();
  
  Wire.begin();
  mpu.initialize();
  if (!mpu.testConnection()) {
    Serial.println("MPU6050 connection failed!");
  } else {
    Serial.println("MPU6050 connected");
  }
  
  WiFi.softAP(ssid, password);
  Serial.println("AP started");
  Serial.print("IP: ");
  Serial.println(WiFi.softAPIP());
  
  for (int i = 0; i < 10; i++) {
    gyroZHistory[i] = 0;
  }
  
  server.on("/", handleRoot);
  server.on("/index.html", handleIndex);
  server.on("/app.js", handleAppJS);
  server.on("/sse", handleSSE);
  
  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  server.handleClient();
  
  if (Serial.available() > 0) {
    String input = Serial.readStringUntil('\n');
    input.trim();
    if (input.startsWith("T:")) {
      throttle = input.substring(2).toFloat();
    } else if (input.startsWith("R:")) {
      rpm = input.substring(2).toFloat();
    }
  }
}

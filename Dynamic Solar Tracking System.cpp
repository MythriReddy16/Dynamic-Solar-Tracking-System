//test 1



#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Servo.h>
#include <time.h>

#define LDR_LEFT 14   // D5
#define LDR_RIGHT 12  // D6
#define SERVO_PIN 13  // D7

const char* ssid = "advprojectshub";     // WiFi SSID
const char* password = "advprojectshub";       // WiFi password

LiquidCrystal_I2C lcd(0x27, 16, 2);
Servo solarServo;
ESP8266WebServer server(80);

int servoPos = 90;
int stepSize = 4;
String statusStr;
String logData = "";

// Time settings (IST = UTC + 5:30)
const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 19800; // +5:30
const int daylightOffset_sec = 0;

void setup() {
  Serial.begin(9600);
  pinMode(LDR_LEFT, INPUT);
  pinMode(LDR_RIGHT, INPUT);

  solarServo.attach(SERVO_PIN);
  solarServo.write(servoPos);

  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Solar Tracker");
  lcd.setCursor(0, 1);
  lcd.print("Connecting WiFi");
  
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("WiFi Connected");
  lcd.setCursor(0, 1);
  lcd.print(WiFi.localIP());
  delay(2000);
  lcd.clear();

  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

  server.on("/", handleRoot);
  server.on("/data", handleData);
  server.begin();
  Serial.println("Web Server Started");
}

void loop() {
  server.handleClient();
  trackSun();
  delay(1000);
}

void trackSun() {
  int leftLight = digitalRead(LDR_LEFT);
  int rightLight = digitalRead(LDR_RIGHT);

  if (leftLight == LOW && rightLight == HIGH) {
    if (servoPos > 0) servoPos -= stepSize;
    statusStr = "LEFT BRIGHT";
  } 
  else if (rightLight == LOW && leftLight == HIGH) {
    if (servoPos < 180) servoPos += stepSize;
    statusStr = "RIGHT BRIGHT";
  } 
  else {
    statusStr = "BALANCED";
  }

  solarServo.write(servoPos);

  lcd.setCursor(0, 0);
  lcd.print("L:");
  lcd.print(leftLight);
  lcd.print(" R:");
  lcd.print(rightLight);
  lcd.print("   ");

  lcd.setCursor(0, 1);
  lcd.print(statusStr);
  lcd.print(" ");
  lcd.setCursor(13, 1);
  lcd.print(servoPos);
  lcd.print("  ");

  String timeStamp = getTimeStamp();
  logData = "<tr><td>" + timeStamp + "</td><td>" + String(leftLight) + "</td><td>" + String(rightLight) + "</td><td>" + String(servoPos) + "</td><td>" + statusStr + "</td></tr>" + logData;
}

String getTimeStamp() {
  time_t now;
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    return "N/A";
  }
  char buffer[30];
  strftime(buffer, sizeof(buffer), "%d-%m-%Y %H:%M:%S", &timeinfo);
  return String(buffer);
}

void handleRoot() {
  String html = "<!DOCTYPE html><html><head><title>Solar Tracker</title>";
  html += "<meta name='viewport' content='width=device-width,initial-scale=1'>";
  html += "<style>body{font-family:Arial;background:#f8f9fa;text-align:center;}table{border-collapse:collapse;width:95%;margin:auto;}th,td{border:1px solid #ccc;padding:8px;}th{background:#007bff;color:#fff;}tr:nth-child(even){background:#e9ecef;}h1{color:#007bff;}</style>";
  html += "<script>function loadData(){fetch('/data').then(r=>r.text()).then(d=>{document.getElementById('log').innerHTML=d;});setTimeout(loadData,2000);}window.onload=loadData;</script></head><body>";
  html += "<h1>Solar Tracking System</h1>";
  html += "<table><tr><th>Date-Time</th><th>LDR1</th><th>LDR2</th><th>Servo</th><th>Status</th></tr><tbody id='log'></tbody></table>";
  html += "</body></html>";
  server.send(200, "text/html", html);
}

void handleData() {
  server.send(200, "text/html", logData);
}
#include <WebServer.h>
#include <WiFi.h>
#include <esp32cam.h>
#include <Arduino.h>
#include <stdlib.h>

const char* WIFI_SSID = "YgpunyaWIBU";
const char* WIFI_PASS = "Voldigoad93";

WebServer server(80);

const int in1 = 12;  // Pin kontrol untuk gearbox 1
const int in2 = 13;  // Pin kontrol untuk gearbox 1
const int in3 = 14;  // Pin kontrol untuk gearbox 2
const int in4 = 15;  // Pin kontrol untuk gearbox 2
const int ledPin = 4; // Pin LED indikator

static auto loRes = esp32cam::Resolution::find(320, 240);
static auto midRes = esp32cam::Resolution::find(350, 530);
static auto hiRes = esp32cam::Resolution::find(800, 600);

void serveJpg() {
  auto frame = esp32cam::capture();
  if (frame == nullptr) {
    Serial.println("CAPTURE FAIL");
    server.send(503, "", "");
    return;
  }
  Serial.printf("CAPTURE OK %dx%d %db\n", frame->getWidth(), frame->getHeight(), static_cast<int>(frame->size()));

  server.setContentLength(frame->size());
  server.send(200, "image/jpeg");
  WiFiClient client = server.client();
  frame->writeTo(client);
}

void handleJpgLo() {
  if (!esp32cam::Camera.changeResolution(loRes)) {
    Serial.println("SET-LO-RES FAIL");
  }
  serveJpg();
}

void handleJpgHi() {
  if (!esp32cam::Camera.changeResolution(hiRes)) {
    Serial.println("SET-HI-RES FAIL");
  }
  serveJpg();
}

void handleJpgMid() {
  if (!esp32cam::Camera.changeResolution(midRes)) {
    Serial.println("SET-MID-RES FAIL");
  }
  serveJpg();
}

void moveForward() {
  digitalWrite(in1, HIGH);
  digitalWrite(in2, LOW); // Move forward gearbox 1
  digitalWrite(in3, HIGH);
  digitalWrite(in4, LOW); // Move forward gearbox 2
  delay(10000); // Move forward for 10 seconds
  digitalWrite(in1, LOW);
  digitalWrite(in2, LOW); // Stop gearbox 1
  digitalWrite(in3, LOW);
  digitalWrite(in4, LOW); // Stop gearbox 2
}

void handleAdaSampah() {
  Serial.println("Received /ada_sampah request");
  digitalWrite(ledPin, HIGH); // Menyalakan LED
  moveForward();
  digitalWrite(ledPin, LOW); // Mematikan LED
  server.send(200, "text/plain", "Boat moved forward for 10 seconds");
}

void setup() {
  Serial.begin(115200);
  Serial.println();

  pinMode(in1, OUTPUT);
  pinMode(in2, OUTPUT);
  pinMode(in3, OUTPUT);
  pinMode(in4, OUTPUT);
  pinMode(ledPin, OUTPUT); // Mengatur pin LED sebagai output

  digitalWrite(in1, LOW);
  digitalWrite(in2, LOW);
  digitalWrite(in3, LOW);
  digitalWrite(in4, LOW);
  digitalWrite(ledPin, LOW); // Memastikan LED mati saat awal

  {
    using namespace esp32cam;
    Config cfg;
    cfg.setPins(pins::AiThinker);
    cfg.setResolution(hiRes);
    cfg.setBufferCount(5);
    cfg.setJpeg(80);

    bool ok = Camera.begin(cfg);
    Serial.println(ok ? "CAMERA OK" : "CAMERA FAIL");
    Serial.println("test0");
  }

  Serial.println("test1");
  WiFi.persistent(false);
  Serial.println("test2");
  WiFi.mode(WIFI_STA);
  Serial.println("test3");
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.println("test4");
  Serial.println("Connecting to Wifi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("Connected to Wifi");
  Serial.print("http://");
  Serial.println(WiFi.localIP());
  Serial.println("  /cam-lo.jpg");
  Serial.println("  /cam-hi.jpg");
  Serial.println("  /cam-mid.jpg");
  Serial.println("  /ada_sampah");

  server.on("/cam-lo.jpg", handleJpgLo);
  server.on("/cam-hi.jpg", handleJpgHi);
  server.on("/cam-mid.jpg", handleJpgMid);
  server.on("/ada_sampah", handleAdaSampah);

  server.begin();
}

// Function to check WiFi connection and reconnect if disconnected
void checkWiFi() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi lost connection. Reconnecting...");
    WiFi.disconnect();
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
    }
    Serial.println("\nWiFi reconnected");
  }
}

void moveGearboxRandomly() {
  // Generate a random value between 0 and 2 to stop 0, 1, or 2 gearboxes
  int stopGearbox = random(3);  // 0: no stop, 1: stop gearbox 1, 2: stop gearbox 2

  // Set control pins for gearbox 1 to move forward or stop
  if (stopGearbox == 1) {
    digitalWrite(in1, LOW);
    digitalWrite(in2, LOW); // Stop gearbox 1
  } else {
    digitalWrite(in1, HIGH);
    digitalWrite(in2, LOW); // Move forward gearbox 1
  }

  // Set control pins for gearbox 2 to move forward or stop
  if (stopGearbox == 2) {
    digitalWrite(in3, LOW);
    digitalWrite(in4, LOW); // Stop gearbox 2
  } else {
    digitalWrite(in3, HIGH);
    digitalWrite(in4, LOW); // Move forward gearbox 2
  }

  // Print control pin status
  Serial.print("Gearbox 1 - IN1: ");
  Serial.print(digitalRead(in1));
  Serial.print(" IN2: ");
  Serial.println(digitalRead(in2));
  Serial.print("Gearbox 2 - IN3: ");
  Serial.print(digitalRead(in3));
  Serial.print(" IN4: ");
  Serial.println(digitalRead(in4));
}

void loop() {
  server.handleClient();
  checkWiFi();
  moveGearboxRandomly();
  delay(5000); // Delay 5 seconds before moving again
}


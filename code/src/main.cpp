#include <Arduino.h>

#include "EspWizLight.h"
#include "WiFi.h"
#include <Encoder.h>
#include <TM1637Display.h>
#include <WiFiUdp.h>
#include <time.h>

#include "UI.h"

// #define BUILTIN_LED 15 // Already defined
const char* SSID = "";
const char* PASSWORD = "";
const char* NTP_SERVER = "dk.pool.ntp.org";
const char* MY_TZ = "CET-1CEST,M3.5.0,M10.5.0/3";
const int I2C_SDA = 33;  // these are also the pins of the hardware i2c
const int I2C_SCL = 35;
const int ENCODER_SW = 33;   // 39;  // this is probably for the press button
const int ENCODER_DT = 18;   // 3;  // 37;
const int ENCODER_CLK = 16;  // 18;
const int LED_CLK = 7;
const int LED_DIO = 5;
const int DHT_PIN = 9;
const int BUZZER_PIN = 3;  // 35;
int BUTTON_PINS[3] = {12, 11, 9};

WizLight light;

UI ui(ENCODER_DT, ENCODER_CLK, ENCODER_SW, LED_CLK, LED_DIO, BUZZER_PIN,
      BUTTON_PINS);  // todo: add buttons

void connectWifi(const char* ssid, const char* password);

// #################### SETUP ####################
void setup() {
  // delay(1000);
  Serial.begin(115200);
  pinMode(BUILTIN_LED, OUTPUT);
  connectWifi(SSID, PASSWORD);
  configTime(0, 0, NTP_SERVER);
  // set the time zone (has to be done after connecting to ntp)
  setenv("TZ", MY_TZ, 1);  //  Now adjust the time zone
  tzset();

  time_t now;
  time(&now);
  Serial.print("Waiting for NTP ");
  while (now < 10000) {
    delay(100);
    digitalWrite(BUILTIN_LED, HIGH);
    delay(100);
    digitalWrite(BUILTIN_LED, LOW);
    Serial.print(".");
    time(&now);
  }
  Serial.println();

  WizLight* lights = new WizLight[8];
  int nLights = WizLight::discoverLights(lights, sizeof(lights), 5);
  Serial.println("Found " + String(nLights) + " lights");
  if (nLights <= 0) {
    // dont continue
    // while (true) {
    //   Serial.println("Error: no lights found");
    //   delay(1000);
    // }
  }
  // TODO: move this into UI?
  for (int i = 0; i < nLights; i++) {
    lights[i].pullConfig();
    Serial.println("Light " + String(i) + ": " + lights[i].getMac());
  }
  // light = lights[0];

  // pinMode(SW, INPUT);
  // attachInterrupt(digitalPinToInterrupt(SW), interrupt, CHANGE);

  ui.begin();
  for (int i = 0; i < nLights; i++) {
    if (lights[i].getMac() == "444f8eb3c77a") {
      ui.registerLight(&lights[i], 0);
    }
    if (lights[i].getMac() == "d8a011bb76ab") {
      ui.registerLight(&lights[i], 1);
    }
    if (lights[i].getMac() == "cc4085082f12") {
      ui.registerLight(&lights[i], 2);
    }
  }

  Serial.println("loop");

}

// #################### LOOP ####################
unsigned long loopIter = 0;
int cOffset = 0;
time_t lastLoop = 0;
void loop() {
  time_t now = millis();
  time_t dt = now - lastLoop;
  lastLoop = now;
  loopIter += 1;

  ui.update();
}

void connectWifi(const char* ssid, const char* password) {
  Serial.print("Connecting to WiFi ");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(100);
    digitalWrite(BUILTIN_LED, HIGH);
    delay(100);
    digitalWrite(BUILTIN_LED, LOW);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
}

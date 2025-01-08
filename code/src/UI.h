#pragma once

#include <Arduino.h>
#include <Encoder.h>
#include <EspWizLight.h>
#include <TM1637Display.h>
// #include <U8g2lib.h>
#include <sunset.h>
#include <time.h>

#include "WiFi.h"

enum UiMode { TIME, DIMMING, TEMPERATURE, HUE, SET_ALARM, ALARM };

struct SunriseState {
  int time;  // time relative to alarm in minutes
  LightConfig lightConfigs[3];

  static LightConfig interpolateLightConfigs(LightConfig l, LightConfig r,
                                             float x) {
    LightConfig m;
    m.state = l.state || r.state;
    m.r = (1. - x) * l.r + x * r.r;
    m.g = (1. - x) * l.g + x * r.g;
    m.b = (1. - x) * l.b + x * r.b;
    m.c = (1. - x) * l.c + x * r.c;
    m.w = (1. - x) * l.w + x * r.w;
    m.dimming = (1. - x) * l.dimming + x * r.dimming;
    m.scene = NO_SCENE;
    m.mode = RGBCW_MODE;
    return m;
  };

  static SunriseState interpolateSunriseStates(SunriseState l, SunriseState r,
                                               float x) {
    SunriseState m((1. - x) * l.time + x * r.time);
    for (int i = 0; i < 3; i++) {
      m.lightConfigs[i] =
          interpolateLightConfigs(l.lightConfigs[i], r.lightConfigs[i], x);
    }
    return m;
  }

  static LightConfig offLight() {
    LightConfig l = LightConfig();
    l.state = false;
    l.dimming = 20;
    l.mode = RGBCW_MODE;
    return l;
  };
  static LightConfig rgbcwLight(int r, int g, int b, int c, int w,
                                int dimming) {
    LightConfig l = LightConfig();
    l.r = r;
    l.g = g;
    l.b = b;
    l.c = c;
    l.w = w;
    l.dimming = dimming;
    l.state = true;
    l.mode = RGBCW_MODE;
    return l;
  };
  static LightConfig tempLight(int temp, int dimming) {
    LightConfig l = LightConfig();
    l.temperature = temp;
    l.dimming = dimming;
    l.state = true;
    l.mode = TEMPERATURE_MODE;
    return l;
  }
  SunriseState(int _time, LightConfig light0, LightConfig light1,
               LightConfig light2) {
    time = _time;
    lightConfigs[0] = light0;
    lightConfigs[1] = light1;
    lightConfigs[2] = light2;
  };
  SunriseState(int _time, LightConfig confs[3]) {
    time = _time;
    for (int i = 0; i < 3; i++) {
      lightConfigs[i] = confs[i];
    }
  };
  SunriseState(int _time) {
    time = _time;
    for (int i = 0; i < 3; i++) {
      lightConfigs[i] = LightConfig();
    }
  };
};

class UI {
 private:
  const unsigned long CLICK_DEAD_TIME = 200;
  const unsigned long DOUBLE_CLICK_TIME = 500;
  const unsigned long MODE_EXIT_TIME = 3000;
  const unsigned long FAST_ENCODER_CHANGE = 50;
  const int SNOOZE_TIME = 5;  // minutes
  const int ENCODER_DT_PIN, ENCODER_CLK_PIN, ENCODER_SW_PIN;
  //   const int OLED_R0_PIN;
  const int LED_CLK_PIN, LED_DIO_PIN;
  const int BUZZER_PIN;
  int BUTTON_PINS[3];
  Encoder encoder;
  //   U8G2_SSD1306_128X64_NONAME_F_HW_I2C
  //   display;  //(U8G2_R0); //, I2C_SCL, I2C_SDA); // works?
  TM1637Display ledDisplay;
  tm timeNow;
  WizLight* lights[3];
  bool lightsRegistered[3] = {false, false, false};
  bool lightsSelected[3] = {false, false, false};

  unsigned long lastEncoderChange;
  unsigned long lastEncoderButtonPress;
  unsigned long lastButtonPress[3];
  unsigned long lastLastButtonPress[3];
  unsigned long lastLastLastButtonPress[3];
  UiMode uiMode = TIME;
  unsigned long lastModeChange = 0;
  unsigned long lastSunriseLightUpdate = 0;
  int globalDimming = 40;
  int globalTemperature = 2400;
  int ledBrightnes = 0;
  int alarmTime = 0;  // alarm time in minutes
  bool alarmSet = false;
  bool buzzerOn = false;
  int lastEncoderState = 0;
  SunSet sunsetCalc;

  void showTimeOnLed();
  void updateTime();
  void updateBuzzer();
  static void forwardEncoderButtonChange(void* instance);
  static void forwardButton0Change(void* instance);
  static void forwardButton1Change(void* instance);
  static void forwardButton2Change(void* instance);
  void onEncoderChange(int delta);
  void onButtonPress(int index);
  void checkButtonClick(int index);
  void resetButtonTimers(int index);
  void encoderButtonClick();
  void buttonClick(int index);
  void buttonDoubleClick(int index);
  void buttonTrippleClick(int index);
  void adjustDimming(int delta);
  void adjustTemperature(int delta);
  void showAlarmTimeOnLed();
  void updateSunrise();
  void setUiMode(UiMode);
  time_t calcSunriseTime();
  bool sunriseLightsNeeded();

 public:
  const int NUM_SUNRISE_STATES = 5;
  SunriseState sunriseStates[5] = {
      SunriseState(-30,                       //
                   SunriseState::offLight(),  //
                   SunriseState::offLight(),  //
                   SunriseState::offLight()),
      SunriseState(-25,  //
                   SunriseState::rgbcwLight(90, 0, 0, 0, 20, 20),
                   SunriseState::offLight(),  //
                   SunriseState::offLight()),
      SunriseState(-20,  //
                   SunriseState::rgbcwLight(90, 0, 0, 0, 50, 40),
                   SunriseState::rgbcwLight(90, 0, 0, 20, 50, 40),
                   SunriseState::offLight()),
      SunriseState(-15,  //
                   SunriseState::rgbcwLight(90, 0, 0, 0, 60, 60),
                   SunriseState::rgbcwLight(90, 0, 0, 20, 50, 60),
                   SunriseState::rgbcwLight(90, 0, 0, 00, 50, 20)),
      SunriseState(-00,  //
                   SunriseState::rgbcwLight(90, 0, 0, 0, 60, 60),
                   SunriseState::rgbcwLight(90, 0, 0, 20, 50, 60),
                   SunriseState::rgbcwLight(90, 0, 0, 20, 80, 50)),
  };
  SunriseState finalSunriseState =
      SunriseState(00,                                 //
                   SunriseState::tempLight(2400, 50),  //
                   SunriseState::tempLight(2400, 50),  //
                   SunriseState::tempLight(2400, 50));
  void setSunriseState(SunriseState state);  // todo make private again
  UI(int encoderDtPin, int encoderClkPin, int encoderSwPin, int ledClkPin,
     int ledDioPin, int buzzerPin, int buttonPins[3]);
  void begin();
  void update();
  void onEncoderButtonChange();
  void onButtonChange(int index);
  void registerLight(WizLight* light, int index);
};
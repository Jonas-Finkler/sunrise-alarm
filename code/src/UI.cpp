

#include "UI.h"

UI::UI(int encoderDtPin, int encoderClkPin, int encoderSwPin, int ledClkPin,
       int ledDioPin, int buzzerPin, int buttonPins[3])
    : ENCODER_DT_PIN(encoderDtPin),
      ENCODER_CLK_PIN(encoderClkPin),
      ENCODER_SW_PIN(encoderSwPin),
      LED_CLK_PIN(ledClkPin),
      LED_DIO_PIN(ledDioPin),
      BUZZER_PIN(buzzerPin),
      encoder(encoderDtPin, encoderClkPin),
      ledDisplay(ledClkPin, ledDioPin) {
  for (int i = 0; i < 3; i++) {
    BUTTON_PINS[i] = buttonPins[i];
  }
};

void UI::forwardEncoderButtonChange(void* instance) {
  UI* self = (UI*)instance;
  self->onEncoderButtonChange();
}

void UI::forwardButton0Change(void* instance) {
  UI* self = (UI*)instance;
  self->onButtonChange(0);
}
void UI::forwardButton1Change(void* instance) {
  UI* self = (UI*)instance;
  self->onButtonChange(1);
}
void UI::forwardButton2Change(void* instance) {
  UI* self = (UI*)instance;
  self->onButtonChange(2);
}

void UI::onEncoderButtonChange() {
  bool encoderButtonState = !digitalRead(ENCODER_SW_PIN);
  if (encoderButtonState) {
    lastEncoderButtonPress = millis();
  }
}

void UI::encoderButtonClick() {
  Serial.println("Encoder button click");
  if (uiMode == TIME) {
    alarmTime =
        (timeNow.tm_hour * 60 + 5 * (timeNow.tm_min / 5) + 5) % (24 * 60);
    alarmSet = false;
    setUiMode(SET_ALARM);
  } else if (uiMode == SET_ALARM) {
    setUiMode(TIME);
    alarmSet = true;
  }
}

void UI::onButtonPress(int index) {
  unsigned long now = millis();
  lastLastLastButtonPress[index] = lastLastButtonPress[index];
  lastLastButtonPress[index] = lastButtonPress[index];
  lastButtonPress[index] = now;
}

void UI::onButtonChange(int index) {
  // bool state = touchInterruptGetLastStatus(BUTTON_PINS[index]);
  bool state =
      true;  // interrupt is attached to rising, so the state is always up
  // Serial.printf("Button %i changed: ", index);
  // Serial.println(index, state);

  if (state) {
    unsigned long now = millis();
    unsigned long dt = now - lastButtonPress[index];
    if (dt > CLICK_DEAD_TIME) {  // dead time for ringing
      onButtonPress(index);
    }
  }
}

void UI::buttonClick(int index) {
  Serial.printf("Button %i click\n", index);
  switch (uiMode) {
    case TIME: {  // toggle light
      if (!lightsRegistered[index]) return;
      lights[index]->pullConfig();
      bool state = !lights[index]->getState();
      lights[index]->setState(state);
      lights[index]->pushConfig();
      break;
    }

    case DIMMING:
    case TEMPERATURE: {  // add light to selection
      if (!lightsRegistered[index]) return;
      lightsSelected[index] = !lightsSelected[index];
      lastModeChange = millis();
      bool anyLightsSelected = false;
      for (int i = 0; i < 3; i++) {
        anyLightsSelected |= lightsSelected[i];
      }
      if (!anyLightsSelected) {  // no lights selected anymore. Go back to
                                 // showing the time
        setUiMode(TIME);
      }
      break;
    }

    case ALARM: {  // snooze
      Serial.println("snooze");
      setUiMode(TIME);
      alarmSet = true;
      Serial.println("Old alarm time: " + String(alarmTime));
      int timeNowMinutes = timeNow.tm_hour * 60 + timeNow.tm_min;
      alarmTime =
          (timeNowMinutes + SNOOZE_TIME) % (24 * 60);  // add five minutes
      Serial.println("New alarm time: " + String(alarmTime));
      break;
    }

    default:
      break;
  }
}

void UI::buttonDoubleClick(int index) {
  Serial.printf("Button %i double click\n", index);
  switch (uiMode) {
    case TIME:
      if (!lightsRegistered[index]) return;
      lightsSelected[index] = true;
      lights[index]->pullConfig();
      globalDimming = lights[index]->getDimming();
      setUiMode(DIMMING);
      break;

    case ALARM:
      setUiMode(TIME);  // turn alarm off
      alarmSet = false;
      // to make sure user knows alarm is off now
      for (int i = 1; i < 4; i++) {
        digitalWrite(BUZZER_PIN, HIGH);
        delay(50);
        digitalWrite(BUZZER_PIN, LOW);
        delay(200);
      }
      // turn lights to normal
      if (sunriseLightsNeeded()) {
        setSunriseState(finalSunriseState);
      }
      break;

    default:
      break;
  }
  if (uiMode == TIME) {
  }
}

void UI::buttonTrippleClick(int index) {
  Serial.printf("Button %i tripple click\n", index);
  if (!lightsRegistered[index]) return;
  if (uiMode == TIME) {
    lightsSelected[index] = true;
    lights[index]->pullConfig();
    globalTemperature = lights[index]->getTemperature();
    if (globalTemperature < WizLight::TEMPERATURE_MIN) {
      globalTemperature = WizLight::TEMPERATURE_MIN + 400;
    }
    setUiMode(TEMPERATURE);
  }
}

void UI::onEncoderChange(int delta) {
  unsigned long now = millis();
  switch (uiMode) {
    case DIMMING:
      adjustDimming(delta * 5);
      break;

    case TEMPERATURE:
      adjustTemperature(delta * 50);
      break;

    case SET_ALARM:
      if (now - lastEncoderChange < FAST_ENCODER_CHANGE) {
        alarmTime += delta * 20;
      } else {
        alarmTime += delta * 5;
      }
      alarmTime = alarmTime % (24 * 60);
      break;

    default:  // TIME or ALARM
      ledBrightnes = max(-1, min(ledBrightnes + delta, 7));
      if (ledBrightnes < 0) {
        ledDisplay.setBrightness(0, false);
      } else {
        ledDisplay.setBrightness(ledBrightnes, true);
      }

      break;
  }
  lastModeChange = now;
  lastEncoderChange = now;
}

void UI::adjustDimming(int delta) {
  globalDimming =
      min(max(WizLight::DIM_MIN, globalDimming + delta), WizLight::DIM_MAX);
  for (int lightIndex = 0; lightIndex < 3; lightIndex++) {
    if (lightsSelected[lightIndex] && lightsRegistered[lightIndex]) {
      lights[lightIndex]->setState(true);
      lights[lightIndex]->setDimming(globalDimming);
      lights[lightIndex]->pushConfig();
    }
  }
}

void UI::adjustTemperature(int delta) {
  globalTemperature =
      min(max(WizLight::TEMPERATURE_MIN, globalTemperature + delta),
          WizLight::TEMPERATURE_MAX);
  for (int lightIndex = 0; lightIndex < 3; lightIndex++) {
    if (lightsSelected[lightIndex] && lightsRegistered[lightIndex]) {
      lights[lightIndex]->setState(true);
      lights[lightIndex]->setTemperature(globalTemperature);
      lights[lightIndex]->pushConfig();
    }
  }
}

void UI::begin() {
  ledBrightnes = 0;
  ledDisplay.setBrightness(ledBrightnes);  // 0 - 7

  pinMode(ENCODER_SW_PIN, INPUT);
  attachInterruptArg(digitalPinToInterrupt(ENCODER_SW_PIN),
                     forwardEncoderButtonChange, this, CHANGE);

  // todo: attach button pins
  for (int i = 0; i < 3; i++) {
    lastButtonPress[i] = 0;
    pinMode(BUTTON_PINS[i], INPUT_PULLUP);
  }
  attachInterruptArg(BUTTON_PINS[0], forwardButton0Change, this, RISING);
  attachInterruptArg(BUTTON_PINS[1], forwardButton1Change, this, RISING);
  attachInterruptArg(BUTTON_PINS[2], forwardButton2Change, this, RISING);
  // touchAttachInterruptArg(BUTTON_PINS[0], forwardButton0Change, this, 2500);

  pinMode(BUZZER_PIN, OUTPUT);

  updateTime();
}

// return the sunrise time in minutes
time_t UI::calcSunriseTime() {
  // Aalborg
  sunsetCalc.setPosition(57.05341573851677, 9.906116686625493, 0.0);
  time_t now;
  time(&now);
  struct tm* gmt;
  gmt = gmtime(&now);
  int tzoffset = (timeNow.tm_hour - gmt->tm_hour) % 24;
  sunsetCalc.setTZOffset(tzoffset);
  sunsetCalc.setCurrentDate(gmt->tm_year + 1990, gmt->tm_mon + 1, gmt->tm_mday);
  double sunrise = sunsetCalc.calcSunrise();
  return (time_t)sunrise;
}

bool UI::sunriseLightsNeeded() {
  time_t sunriseTime = calcSunriseTime();  // when the actual sun will rise
  int timeNowMinutes = timeNow.tm_hour * 60 + timeNow.tm_min;
  int timeToAlarm = (timeNowMinutes - alarmTime + 12 * 60) % (24 * 60) -
                    12 * 60;  // get result from -12h to +12h
  return alarmTime < sunriseTime + 45;
}

void UI::resetButtonTimers(int index) {
  lastButtonPress[index] = 0;
  lastLastButtonPress[index] = 0;
  lastLastLastButtonPress[index] = 0;
}

// checks if button has been clicked since last update and performs necessary
// action
void UI::checkButtonClick(int index) {
  unsigned long now = millis();
  switch (uiMode) {
    case TIME:  // allow tripple clicks
      if (lastButtonPress[index] != 0 &&
          (now - lastButtonPress[index]) > CLICK_DEAD_TIME) {
        if (now - lastButtonPress[index] >
            DOUBLE_CLICK_TIME) {  // done waiting for more presses
          if (now - lastLastButtonPress[index] < 2 * DOUBLE_CLICK_TIME) {
            if (now - lastLastLastButtonPress[index] < 3 * DOUBLE_CLICK_TIME) {
              resetButtonTimers(index);
              buttonTrippleClick(index);
            } else {
              resetButtonTimers(index);
              buttonDoubleClick(index);
            }
          } else {
            resetButtonTimers(index);
            buttonClick(index);
          }
        }
      }
      break;

    case ALARM:  // allow double clicks
      if (lastButtonPress[index] != 0 &&
          (now - lastButtonPress[index]) > CLICK_DEAD_TIME) {
        if (now - lastButtonPress[index] >
            DOUBLE_CLICK_TIME) {  // no need to wait for an extra click
          if (now - lastLastButtonPress[index] < 2 * DOUBLE_CLICK_TIME) {
            resetButtonTimers(index);
            buttonDoubleClick(index);
          } else {
            resetButtonTimers(index);
            buttonClick(index);
          }
        }
      }
      break;

    default:  // only single clicks
      if (lastButtonPress[index] != 0 &&
          (now - lastButtonPress[index]) > CLICK_DEAD_TIME) {
        resetButtonTimers(index);
        buttonClick(index);
      }
      break;
  }

  // encoder
  if (lastEncoderButtonPress != 0 &&
      (now - lastEncoderButtonPress) > CLICK_DEAD_TIME) {
    lastEncoderButtonPress = 0;
    encoderButtonClick();
  }
}

void UI::update() {
  updateTime();

  // buttons
  for (int buttonIndex = 0; buttonIndex < 3; buttonIndex++) {
    checkButtonClick(buttonIndex);
  }

  // encoder
  int encoderState = (int)round(((double)encoder.read()) / 4);
  int encoderDelta = encoderState - lastEncoderState;
  if (encoderDelta != 0) {
    lastEncoderState = encoderState;
    onEncoderChange(encoderDelta);
  }

  // led
  byte lightDots = 0;
  if (lightsSelected[0]) lightDots += 0b10000000;
  if (lightsSelected[1]) lightDots += 0b01000000;
  if (lightsSelected[2]) lightDots += 0b00100000;
  switch (uiMode) {
    case TIME:
    case ALARM:
      showTimeOnLed();
      break;

    case DIMMING:
      // ledDisplay.showNumberDec(globalDimming);
      ledDisplay.showNumberDecEx(globalDimming, lightDots, true);
      break;

    case TEMPERATURE:
      // ledDisplay.showNumberDec(globalTemperature);
      ledDisplay.showNumberDecEx(globalTemperature, lightDots, true);
      break;

    case SET_ALARM:
      showAlarmTimeOnLed();
      break;
  }

  // timeouts
  unsigned long millisNow = millis();
  if (uiMode != TIME && uiMode != ALARM) {  // anything except alarm and time
    if (millisNow - lastModeChange > MODE_EXIT_TIME) {
      setUiMode(TIME);
      for (int i = 0; i < 3; i++) {
        lightsSelected[i] = false;
      }
    }
  }

  // check alarm
  int timeNowMinutes = timeNow.tm_hour * 60 + timeNow.tm_min;
  if ((uiMode != ALARM) && alarmSet) {
    if (timeNowMinutes == alarmTime) {  // assuming refresh rate > 1/minute
      setUiMode(ALARM);
      Serial.println("RING RING");
    }
  }
  if (uiMode == ALARM) {
    if ((timeNowMinutes - alarmTime) % (24 * 60) >
        2 * SNOOZE_TIME) {  // turn off alarm after twice the snooze time
      setUiMode(TIME);
      alarmSet = false;
      // set lights to normal if they have been used
      if (sunriseLightsNeeded()) {
        setSunriseState(finalSunriseState);
      }
    }
  }

  updateBuzzer();
  updateSunrise();
}

void UI::updateSunrise() {
  unsigned long now = millis();
  if (alarmSet &&
      (now - lastSunriseLightUpdate) > 60000) {  // only update lights every 60s
    time_t sunriseTime = calcSunriseTime();  // when the actual sun will rise

    int timeNowMinutes = timeNow.tm_hour * 60 + timeNow.tm_min;
    int timeToAlarm = (timeNowMinutes - alarmTime + 12 * 60) % (24 * 60) -
                      12 * 60;  // get result from -12h to +12h

    if (!sunriseLightsNeeded()) {
      // calculating the actual sunrise is somewhat expensive so we don't do it
      // too often.
      lastSunriseLightUpdate = now;
      return;  // it's already light outside. We dont need the lamps.
      // (also in case the alarm is set to before midnight, it doesn't matter
      // anymore)
    }

    int last = -1;
    int next = -1;
    for (int i = 0; i < NUM_SUNRISE_STATES; i++) {
      int timeToState = sunriseStates[i].time;
      if (sunriseStates[i].time > timeToAlarm) {  // it could be next
        if ((sunriseStates[i].time - timeToAlarm) <
            120) {         // dont look more than two hours ahead
          if (next < 0) {  // better than nothing
            next = i;
          } else {
            if (sunriseStates[i].time < sunriseStates[next].time) {
              next = i;
            }
          }
        }
      } else {  // it could be last
        if ((timeToAlarm - sunriseStates[i].time) <
            120) {         // dont look more than two hours back
          if (last < 0) {  // better than nothing
            last = i;
          } else {
            if (sunriseStates[i].time > sunriseStates[last].time) {
              last = i;
            }
          }
        }
      }
    }
    if (last > 0 && next > 0) {  // its go time
      float x = 1.0 * (timeToAlarm - sunriseStates[last].time) /
                (sunriseStates[next].time - sunriseStates[last].time);
      SunriseState ss = SunriseState::interpolateSunriseStates(
          sunriseStates[last], sunriseStates[next], x);
      setSunriseState(ss);
      lastSunriseLightUpdate = now;
      Serial.println("Updating sunrise");
      Serial.println("last: " + String(last));
      Serial.println("next: " + String(next));
      Serial.println("x: " + String(x));
    }
  }
}

void UI::setSunriseState(SunriseState state) {
  for (int i = 0; i < 3; i++) {
    if (lightsRegistered[i]) {
      lights[i]->setConfig(state.lightConfigs[i]);
      WizResult res = lights[i]->pushConfig();
      switch (res) {
        case TIMEOUT:
          res = lights[i]->pushConfig();  // try again
          break;

        case LIGHT_ERROR:
          Serial.println("Light error sending sunrise state!");

        default:
          break;
      }
      Serial.print(res);
    }
  }
  Serial.println();
}

void UI::updateBuzzer() {
  if (uiMode == ALARM) {
    unsigned long t = millis() % 1000;
    bool buzz = (t < 50);
    if (buzz != buzzerOn) {
      buzzerOn = buzz;
      digitalWrite(BUZZER_PIN, buzzerOn);
    }

  } else {
    if (buzzerOn) {
      buzzerOn = false;
      digitalWrite(BUZZER_PIN, false);
    }
  }
}

void UI::updateTime() { getLocalTime(&timeNow); }

void UI::showAlarmTimeOnLed() {
  int minutes = alarmTime % 60;
  int hours = ((alarmTime - minutes) / 60) % 24;
  int timeInt = hours * 100 + minutes;
  ledDisplay.showNumberDecEx(timeInt, 0b01000000,
                             true);  // puts a dot in the middle
}

void UI::showTimeOnLed() {
  int timeInt = timeNow.tm_hour * 100 + timeNow.tm_min;
  if (alarmSet) {
    if (millis() / 1000 % 5 < 2) {
      ledDisplay.showNumberDecEx(timeInt, 0b00000000, true);  // no dot
    } else {
      ledDisplay.showNumberDecEx(timeInt, 0b01000000,
                                 true);  // puts a dot in the middle
    }
  } else {
    ledDisplay.showNumberDecEx(timeInt, 0b01000000,
                               true);  // puts a dot in the middle
  }
}

void UI::setUiMode(UiMode mode) {
  uiMode = mode;
  lastModeChange = millis();
}

void UI::registerLight(WizLight* light, int index) {
  lights[index] = light;
  lightsRegistered[index] = true;
}
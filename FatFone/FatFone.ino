#include <Adafruit_ILI9341.h> // Libraries
#include <Adafruit_FONA.h>
#include <Adafruit_FT6206.h>
#include <SoftwareSerial.h>

Adafruit_FT6206 ts = Adafruit_FT6206(); // Library initializations

#define TFT_CS 10
#define TFT_DC 9
#define TFT_BL 5
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC);

#define LOCK_PIN 7

#define FONA_RX 2
#define FONA_TX 3
#define FONA_RST -1
#define FONA_RI 6
#define FONA_KEY 8
SoftwareSerial fonaSS = SoftwareSerial(FONA_TX, FONA_RX);
SoftwareSerial *fonaSerial = &fonaSS;
Adafruit_FONA fona = Adafruit_FONA(FONA_RST);

#define lightgrey ILI9341_LIGHTGREY // System definitions/variables
#define green ILI9341_GREEN
#define black ILI9341_BLACK
#define blue ILI9341_BLUE
#define navy ILI9341_NAVY
#define white ILI9341_WHITE
#define red ILI9341_RED
#define cyan ILI9341_CYAN
#define darkgreen ILI9341_DARKGREEN
#define maroon ILI9341_MAROON
#define greenyellow ILI9341_GREENYELLOW
#define darkgrey ILI9341_DARKGREY

#define LOCKTIME -2
#define KEYPAD -1
#define LOCKSCREEN 0
#define MENU 1
#define SETTINGS 2
#define PHONE 3
#define MESSAGES 4
#define CONTACTS 5
#define PONG 6

#define LSTextX 60
#define LSTextY 20

#define clockX 75
#define clockY 12

#define batteryX 190
#define batteryY 4

bool DEMO = false; // Does the Fona actually make calls and stuff or is it just a demo?

int password[] = {0, 0, 0, 0}; // Default password
int givenPassword[] = {' ', ' ', ' ', ' '};

int bl = 12;
int volume = 60;
bool screenDimmed = false;
int dimmedBL;
int audio = FONA_EXTAUDIO; // Other audio setting is FONA_HEADSETAUDIO

bool phoneLocked = false;

int screen = 0;
int lastScreen = 1;

int time[] = {12, 00};
long timeTimer = millis();
bool timeUpdated = true;

int numOfAppsP1 = 5;
int appX[] = {20, 20, 20, 20, 20};
int appY[] = {70, 120, 170, 220, 270};
char* appName[] = {"Settings", "Phone", "Messages", "Contacts", "Pong"};
uint16_t appColor[] = {darkgrey, red, darkgreen, navy, black};

long idleTimer;
int idleTimeout = 60000;

long uptime = millis();

bool hasStarted = false;

uint16_t bat;
int batry;
int oldBatry;
long batTimer = millis();

char givenPNumber[10] = {' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' '};

int8_t netStat;
int8_t oldNetStat;

int8_t callStat;

void setup() {
  //Serial.begin(9600); // For debugging/testing coordinates
  pinMode(TFT_BL, OUTPUT);
  pinMode(FONA_KEY, OUTPUT);
  digitalWrite(FONA_KEY, HIGH);
  tft.begin();
  tft.setRotation(0);
  tft.fillScreen(navy);
  tft.fillRoundRect(10, 10, 220, 135, 10, cyan); //Logo
  tft.setTextSize(12);
  tft.setCursor(50, 30);
  tft.setTextColor(black);
  tft.print(F("F"));
  tft.setTextSize(5);
  tft.print(F("at"));
  tft.setCursor(108, 78);
  tft.print(F("ONE"));
  tft.setTextSize(3);
  tft.setCursor(50, 170);
  tft.setTextColor(white);
  tft.print(F("Starting"));
  tft.setCursor(85, 200);
  for (int i = 0; i <= bl; i++) {
    backlight(i);
    delay(20);
  }
  ts.begin(40);
  fonaSerial->begin(4800);
  tft.print(F("."));
  fona.begin(*fonaSerial);
  tft.print(F("."));
  fona.setAudio(audio);
  tft.print(F("."));
  fona.setVolume(volume);
  tft.print(F("."));
  fona.unlockSIM("1234"); // SIM pin code (just an example)
  fona.playToolkitTone(6, 500);
  pinMode(FONA_RI, INPUT);
  pinMode(LOCK_PIN, INPUT_PULLUP);
  tft.setTextWrap(false);
  draw(LOCKSCREEN, 2);
  idleTimer = millis();
  oldBatry = getBattery();
}

void loop() {
  if (phoneLocked) {
    while (phoneLocked) {
      if (ts.touched()) {
        touchHandler(0);
        tft.setTextSize(5);
        tft.setTextColor(black, white);
        tft.setCursor(LSTextX, LSTextY);
        if (givenPassword[0] != ' ') {
          tft.print(givenPassword[0]);
        }
        if (givenPassword[1] != ' ') {
          tft.print(givenPassword[1]);
        }
        if (givenPassword[2] != ' ') {
          tft.print(givenPassword[2]);
        }
        if (givenPassword[3] != ' ') {
          tft.print(givenPassword[3]);
          phoneLocked = !checkIfPasswordCorrect();
          if (phoneLocked) {
            fona.setPWM(2000);
            givenPassword[0] = ' ';
            givenPassword[1] = ' ';
            givenPassword[2] = ' ';
            givenPassword[3] = ' ';
            tft.fillRect(7, 7, 226, 66, white);
            delay(100);
            fona.setPWM(0);
          }
        }
        while (ts.touched()) {}
      }
      if (timeUpdated) {
        drawTime(0, blue);
      }
      if (digitalRead(LOCK_PIN) == 0) {
        lock();
      }
      updateTime();
      if (millis() - idleTimer >= (idleTimeout - 7000)) {
        screenDimmed = true;
        dimmedBL = (bl / 3);
        backlight(dimmedBL);
        if (millis() - idleTimer >= idleTimeout) {
          lock();
          idleTimer = millis();
          screenDimmed = false;
        }
      }
      if (digitalRead(LOCK_PIN) == 0) {
        lock();
      }
    }
    givenPassword[0] = ' ';
    givenPassword[1] = ' ';
    givenPassword[2] = ' ';
    givenPassword[3] = ' ';
    if (hasStarted) {
      draw(lastScreen, 1);
    } else {
      draw(MENU, 1);
      hasStarted = true;
    }
  }
  if (timeUpdated) {
    drawTime(1, blue);
  }
  if (ts.touched()) {
    touchHandler(MENU);
  }
  updateTime();
  if (millis() - idleTimer >= (idleTimeout - 7000)) {
    screenDimmed = true;
    dimmedBL = (bl / 2);
    backlight(dimmedBL);
    if (millis() - idleTimer >= idleTimeout) {
      lock();
      idleTimer = millis();
      screenDimmed = false;
    }
  }
  if (batteryUpdated()) {
    tft.setTextColor(white, blue);
    tft.setTextSize(2);
    tft.setCursor(batteryX, batteryY);
    tft.print(getBattery());
    tft.print(F("%"));
  }
  if (digitalRead(LOCK_PIN) == 0) {
    lock();
  }
  if (checkNetStat() == true) {
    printNetStat();
  }
}

void draw(int a, int b) {
  if (a != -1 && b != 0) {
    for (int i = bl; i >= 0; i--) {
      backlight(i);
      delay(10);
    }
    screen = a;
  }
  switch (a) {
    case LOCKTIME:
      tft.setTextColor(black, cyan);
      tft.setTextSize(6);
      tft.fillRoundRect(18, 10, 200, 65, 8, cyan);
      tft.setCursor(50, 20);
      if (time[0] > 10) {
        tft.setCursor(30, 20);
      }
      tft.print(time[0]);
      tft.print(F(":"));
      if (time[1] < 10) {
        tft.print(F("0"));
      }
      tft.print(time[1]);
      tft.setTextColor(white, blue);
      tft.setTextSize(3);
      tft.setCursor(40, 90);
      tft.print(F("BATT:"));
      if (getBattery() >= 95) {
        tft.print(F("Full"));
      } else {
        tft.print(F(" "));
        tft.print(getBattery());
        tft.print(F("%"));
      }
      break;
    case KEYPAD:
      tft.fillRect(0, 160, 240, 320, white);
      tft.drawFastHLine(0, 159, 240, black);
      tft.drawFastHLine(0, 160, 240, black);

      tft.setTextSize(5);
      tft.setTextColor(black, white);
      tft.setCursor(20, 170);
      tft.print(F("1 2 3 4"));
      tft.setCursor(20, 220);
      tft.print(F("5 6 7 8"));
      tft.setCursor(20, 270);
      tft.print(F("9 0 <<"));

      break;
    case LOCKSCREEN:
      if (b == 2) {
        tft.fillScreen(blue);
      }
      tft.fillRect(5, 5, 230, 70, white);
      tft.drawRect(5, 5, 230, 70, black);
      draw(KEYPAD, 0);
      drawTime(0, blue);
      tft.setTextSize(5);
      tft.setTextColor(black, white);
      tft.setCursor(LSTextX, LSTextY);
      if (givenPassword[0] != ' ') {
        tft.print(givenPassword[0]);
      }
      if (givenPassword[1] != ' ') {
        tft.print(givenPassword[1]);
      }
      if (givenPassword[2] != ' ') {
        tft.print(givenPassword[2]);
      }
      if (givenPassword[3] != ' ') {
        tft.print(givenPassword[3]);
      }
      phoneLocked = true;
      break;
    case MENU:
      tft.fillScreen(cyan);
      tft.fillRect(0, 0, 240, 50, blue);
      for (int i = 0; i < numOfAppsP1; i++) {
        tft.setCursor(appX[i], appY[i]);
        tft.setTextColor(appColor[i], cyan);
        tft.setTextSize(3);
        tft.print(appName[i]);
      }
      drawTime(1, blue);
      lastScreen = MENU;
      tft.setTextColor(white, blue);
      tft.setTextSize(2);
      tft.setCursor(batteryX, batteryY);
      tft.print(getBattery());
      tft.print(F("%"));
      printNetStat();
      break;
    case SETTINGS:
      tft.fillScreen(white);
      tft.fillRect(0, 0, 240, 50, darkgrey);
      drawTime(1, darkgrey);
      tft.setCursor(7, 80);
      tft.setTextColor(black, white);
      tft.setTextSize(2);
      tft.print(F("Backlight"));
      tft.fillRect(130, 70, 100, 50, darkgrey);
      tft.drawRect(130, 70, 50, 50, black);
      tft.drawRect(180, 70, 50, 50, black);
      tft.setCursor(150, 80);
      tft.setTextSize(4);
      tft.setTextColor(black, darkgrey);
      tft.print(F("-"));
      tft.setCursor(200, 80);
      tft.print(F("+"));
      break;
    case PHONE:
      tft.fillScreen(white);
      tft.fillRect(0, 0, 240, 50, red);
      drawTime(1, red);
      tft.setTextSize(2);
      tft.setTextColor(black, white);
      tft.setCursor(20, 80);
      tft.print(F("Answer to call"));
      tft.drawFastHLine(0, 120, 240, darkgrey);
      tft.setCursor(20, 140);
      tft.print(F("End call"));
      tft.drawFastHLine(0, 180, 240, darkgrey);
      tft.setCursor(20, 200);
      tft.print(F("Call to number"));
      break;
    case MESSAGES:
      tft.fillScreen(white);
      tft.fillRect(0, 0, 240, 50, darkgreen);
      drawTime(1, darkgreen);
      break;
    case CONTACTS:
      tft.fillScreen(white);
      tft.fillRect(0, 0, 240, 50, navy);
      drawTime(1, navy);
      break;
    case PONG:
      tft.fillScreen(black);
      for (int i = 0; i <= 240; i += 2) {
        tft.drawPixel(i, 160, white);
      }
      tft.fillRect(80, 310, 80, 10, white);
      break;
  }
  if (a >= 2) {
    lastScreen = a;
  }
  if (a != -1 && b != 0) {
    for (int i = 0; i <= bl; i++) {
      backlight(i);
      delay(10);
    }
  }
}

void drawTime(int a, uint16_t bgColor) {
  if (a == 0) {
    tft.setCursor(37, 100);
    tft.setTextSize(5);
    tft.setTextColor(cyan, bgColor);
    if (time[0] < 10) {
      tft.setCursor(44, 100);
    }
    tft.print(time[0]);
    tft.print(F(":"));
    if (time[1] < 10) {
      tft.print(F("0"));
    }
    tft.print(time[1]);
  } else {
    tft.setCursor(clockX, clockY);
    tft.setTextSize(3);
    tft.setTextColor(white, bgColor);
    if (time[0] < 10) {
      tft.setCursor((clockX + 11), clockY);
    }
    tft.print(time[0]);
    tft.print(F(":"));
    if (time[1] < 10) {
      tft.print(F("0"));
    }
    tft.print(time[1]);
  }
}

void updateTime() {
  if (millis() - timeTimer >= 60000) {
    time[1] += 1;
    if (time[1] >= 60) {
      time[1] = 0;
      time[0] += 1;
    }
    if (time[0] >= 24) {
      time[0] = 0;
    }
    timeUpdated = true;
    timeTimer = millis();
  } else {
    timeUpdated = false;
  }
}

void lock() {
  while (ts.touched()) {}
  if (screenDimmed) {
    for (int i = dimmedBL; i >= 0; i--) {
      backlight(i);
      delay(30);
    }
  } else {
    for (int i = bl; i >= 0; i--) {
      backlight(i);
      delay(30);
    }
  }
  phoneLocked = true;
lockBegin:
  tft.fillScreen(blue);

  while (digitalRead(LOCK_PIN) == 1) {
    updateTime();
  }
  delay(200);
  if (digitalRead(LOCK_PIN) == 0) {
    draw(LOCKTIME, 0);
    for (int i = 0; i <= bl; i++) {
      backlight(i);
      delay(30);
    }
    while (digitalRead(LOCK_PIN) == 0) {
      updateTime();
      if (timeUpdated) {
        tft.setTextColor(black, cyan);
        tft.setTextSize(6);
        tft.setCursor(50, 20);
        if (time[0] > 10) {
          tft.setCursor(30, 20);
        }
        tft.print(time[0]);
        tft.print(F(":"));
        if (time[1] < 10) {
          tft.print(F("0"));
        }
        tft.print(time[1]);
        tft.setTextColor(white, blue);
        tft.setTextSize(3);
        tft.setCursor(40, 90);
        tft.print(F("BATT:"));
        if (batteryUpdated()) {
          if (getBattery() >= 95) {
            tft.print(F("Full"));
          } else {
            tft.print(F(" "));
            tft.print(getBattery());
            tft.print(F("%"));
          }
        }
      }
      delay(1000);
    }
    for (int i = bl; i >= 0; i--) {
      backlight(i);
      delay(30);
    }
    goto lockBegin;
  } else {
    draw(LOCKSCREEN, 0);
  }
  for (int i = 0; i <= bl; i++) {
    backlight(i);
    delay(30);
  }
  while (phoneLocked) {
    if (ts.touched()) {
      touchHandler(0);
      tft.setTextSize(5);
      tft.setTextColor(black, white);
      tft.setCursor(LSTextX, LSTextY);
      if (givenPassword[0] != ' ') {
        tft.print(givenPassword[0]);
      }
      if (givenPassword[1] != ' ') {
        tft.print(givenPassword[1]);
      }
      if (givenPassword[2] != ' ') {
        tft.print(givenPassword[2]);
      }
      if (givenPassword[3] != ' ') {
        tft.print(givenPassword[3]);
        phoneLocked = !checkIfPasswordCorrect();
        if (phoneLocked) {
          fona.setPWM(2000);
          givenPassword[0] = ' ';
          givenPassword[1] = ' ';
          givenPassword[2] = ' ';
          givenPassword[3] = ' ';
          tft.fillRect(7, 7, 226, 66, white);
          delay(100);
          fona.setPWM(0);
        }
      }
      while (ts.touched()) {}
    }
    if (timeUpdated) {
      drawTime(0, blue);
    }
    if (digitalRead(LOCK_PIN) == 0) {
      lock();
    }
    updateTime();
    if (millis() - idleTimer >= (idleTimeout - 7000)) {
      screenDimmed = true;
      dimmedBL = (bl / 3);
      backlight(dimmedBL);
      if (millis() - idleTimer >= idleTimeout) {
        lock();
        idleTimer = millis();
        screenDimmed = false;
      }
    }
  }
  givenPassword[0] = ' ';
  givenPassword[1] = ' ';
  givenPassword[2] = ' ';
  givenPassword[3] = ' ';
  if (hasStarted) {
    draw(lastScreen, 1);
  } else {
    draw(MENU, 1);
    hasStarted = true;
  }
}

void backlight(int a) {
  int b = map(a, 0, 20, 0, 255);
  analogWrite(TFT_BL, b);
}

void touchHandler(int a) {
  idleTimer = millis();
  if (screenDimmed) {
    backlight(bl);
    screenDimmed = false;
  } else {
    TS_Point touchPoint = ts.getPoint();
    touchPoint.x = map(touchPoint.x, 0, 240, 240, 0);
    touchPoint.y = map(touchPoint.y, 0, 320, 320, 0);
    switch (a) {
      case LOCKSCREEN:
        if (touchPoint.x >= 0 && touchPoint.y >= 160 && touchPoint.x <= 75 && touchPoint.y <= 220) {
          insertToGivenPassword(1);
        } else if (touchPoint.x >= 75 && touchPoint.y >= 160 && touchPoint.x <= 120 && touchPoint.y <= 220) {
          insertToGivenPassword(2);
        } else if (touchPoint.x >= 120 && touchPoint.y >= 160 && touchPoint.x <= 185 && touchPoint.y <= 220) {
          insertToGivenPassword(3);
        } else if (touchPoint.x >= 185 && touchPoint.y >= 160 && touchPoint.x <= 240 && touchPoint.y <= 220) {
          insertToGivenPassword(4);
        } else if (touchPoint.x >= 0 && touchPoint.y >= 220 && touchPoint.x <= 75 && touchPoint.y <= 260) {
          insertToGivenPassword(5);
        } else if (touchPoint.x >= 75 && touchPoint.y >= 220 && touchPoint.x <= 120 && touchPoint.y <= 260) {
          insertToGivenPassword(6);
        } else if (touchPoint.x >= 120 && touchPoint.y >= 220 && touchPoint.x <= 186 && touchPoint.y <= 260) {
          insertToGivenPassword(7);
        } else if (touchPoint.x >= 185 && touchPoint.y >= 220 && touchPoint.x <= 240 && touchPoint.y <= 260) {
          insertToGivenPassword(8);
        } else if (touchPoint.x >= 0 && touchPoint.y >= 260 && touchPoint.x <= 75 && touchPoint.y <= 320) {
          insertToGivenPassword(9);
        } else if (touchPoint.x >= 75 && touchPoint.y >= 260 && touchPoint.x <= 120 && touchPoint.y <= 320) {
          insertToGivenPassword(0);
        } else if (touchPoint.x >= 120 && touchPoint.y >= 260 && touchPoint.x <= 240 && touchPoint.y <= 320) {
          insertToGivenPassword(-1);
        } else if (digitalRead(LOCK_PIN) == 0) {
          lock();
        }
        break;
      case MENU:
        if (digitalRead(LOCK_PIN) == 0) {
          lock();
        } else if (touchPoint.x >= 0 && touchPoint.y >= 55 && touchPoint.x <= 170 && touchPoint.y <= 100) {
          settings();
        } else if (touchPoint.x >= 0 && touchPoint.y >= 100 && touchPoint.x <= 170 && touchPoint.y <= 145) {
          phone();
        } else if (touchPoint.x >= 0 && touchPoint.y >= 145 && touchPoint.x <= 170 && touchPoint.y <= 210) {
          messages();
        } else if (touchPoint.x >= 0 && touchPoint.y >= 210 && touchPoint.x <= 170 && touchPoint.y <= 260) {
          contacts();
        } else if (touchPoint.x >= 0 && touchPoint.y >= 260 && touchPoint.x <= 170 && touchPoint.y <= 320) {
          pong();
        }
        break;
    }
  }
  while (ts.touched()) {}
}

void insertToGivenPassword(int a) {
  if (a == -1) {
    if (givenPassword[3] != ' ') {
      givenPassword[3] = ' ';
    } else if (givenPassword[2] != ' ') {
      givenPassword[2] = ' ';
    } else if (givenPassword[1] != ' ') {
      givenPassword[1] = ' ';
    } else if (givenPassword[0] != ' ') {
      givenPassword[0] = ' ';
    }
    tft.fillRect(7, 7, 226, 66, white);
  } else {
    if (givenPassword[0] == ' ') {
      givenPassword[0] = a;
    } else if (givenPassword[1] == ' ') {
      givenPassword[1] = a;
    } else if (givenPassword[2] == ' ') {
      givenPassword[2] = a;
    } else if (givenPassword[3] == ' ') {
      givenPassword[3] = a;
    }
  }
}

bool checkIfPasswordCorrect() {
  if (givenPassword[0] == password[0] && givenPassword[1] == password[1] && givenPassword[2] == password[2] && givenPassword[3] == password[3]) {
    return true;
  } else {
    return false;
  }
}

void slidePage(bool left, uint16_t color) {
  if (left == false) {
    for (int i = 240; i >= 0; i--) {
      tft.drawFastVLine(i, 50, 270, color);
      tft.drawFastVLine((i - 1), 50, 270, black);
      delayMicroseconds(500);
    }
  } else {
    for (int i = 0; i <= 240; i++) {
      tft.drawFastVLine(i, 50, 270, color);
      tft.drawFastVLine((i + 1), 50, 270, black);
      delayMicroseconds(500);
    }
  }
}

void swapAudio() {
  if (audio == FONA_EXTAUDIO) {
    audio = FONA_HEADSETAUDIO;
    fona.setAudio(audio);
  } else {
    audio = FONA_EXTAUDIO;
    fona.setAudio(audio);
  }
}

int getBattery() {
  fona.getBattPercent(&bat);
  return bat;
}

bool batteryUpdated() {
  if (millis() - batTimer >= 10000) {
    batTimer = millis();
    batry = getBattery();
    if (oldBatry != batry) {
      oldBatry = batry;
      return true;
    } else {
      return false;
    }
  } else {
    return false;
  }
}

void settings() {
  draw(SETTINGS, 1);
  bool exit = false;
  TS_Point touchPoint = ts.getPoint();
  touchPoint.x = map(touchPoint.x, 0, 240, 240, 0);
  touchPoint.y = map(touchPoint.y, 0, 320, 320, 0);
  while (exit == false) {
    if (ts.touched()) {
      idleTimer = millis();
      if (screenDimmed) {
        backlight(bl);
        screenDimmed = false;
      } else {
        TS_Point touchPoint = ts.getPoint();
        touchPoint.x = map(touchPoint.x, 0, 240, 240, 0);
        touchPoint.y = map(touchPoint.y, 0, 320, 320, 0);
        if (touchPoint.y <= 50) {
          exit = true;
        }
      }
    }
    updateTime();
    if (timeUpdated) {
      drawTime(1, darkgrey);
      timeUpdated = false;
    }
    if (millis() - idleTimer >= (idleTimeout - 7000)) {
      screenDimmed = true;
      dimmedBL = (bl / 2);
      backlight(dimmedBL);
      if (millis() - idleTimer >= idleTimeout) {
        lock();
        idleTimer = millis();
        screenDimmed = false;
      }
    }
  }
  exitApp();
}

void phone() {
  draw(PHONE, 1);
  bool exit = false;
  bool subExit = false;
  bool willCall;
  givenPNumber[0] = ' ';
  givenPNumber[1] = ' ';
  givenPNumber[2] = ' ';
  givenPNumber[3] = ' ';
  givenPNumber[4] = ' ';
  givenPNumber[5] = ' ';
  givenPNumber[6] = ' ';
  givenPNumber[7] = ' ';
  givenPNumber[8] = ' ';
  TS_Point touchPoint = ts.getPoint();
  touchPoint.x = map(touchPoint.x, 0, 240, 240, 0);
  touchPoint.y = map(touchPoint.y, 0, 320, 320, 0);
  while (exit == false) {
    if (ts.touched()) {
      idleTimer = millis();
      if (screenDimmed) {
        backlight(bl);
        screenDimmed = false;
      } else {
        TS_Point touchPoint = ts.getPoint();
        touchPoint.x = map(touchPoint.x, 0, 240, 240, 0);
        touchPoint.y = map(touchPoint.y, 0, 320, 320, 0);
        if (touchPoint.y <= 50) {
          exit = true;
        } else if (touchPoint.y <= 120 && touchPoint.y >= 50) {
          fona.pickUp();
        } else if (touchPoint.y <= 180) {
          fona.hangUp();
        } else {
          subExit = false;
          slidePage(false, lightgrey);
          draw(KEYPAD, 0);
          tft.fillRect(5, 60, 230, 50, white);
          tft.drawRect(5, 60, 230, 50, black);
          tft.fillRect(20, 120, 200, 30, green);
          tft.setTextSize(2);
          tft.setTextColor(black, green);
          tft.setCursor(90, 125);
          tft.print(F("Call"));
          while (subExit == false) {
            if (ts.touched()) {
              idleTimer = millis();
              if (screenDimmed) {
                backlight(bl);
                screenDimmed = false;
              } else {
                TS_Point touchPoint = ts.getPoint();
                touchPoint.x = map(touchPoint.x, 0, 240, 240, 0);
                touchPoint.y = map(touchPoint.y, 0, 320, 320, 0);
                if (touchPoint.y <= 50) {
                  subExit = true;
                  willCall = false;
                } else if (touchPoint.x >= 0 && touchPoint.y >= 160 && touchPoint.x <= 75 && touchPoint.y <= 220) {
                  insertToGivenPNumber('1');
                } else if (touchPoint.x >= 75 && touchPoint.y >= 160 && touchPoint.x <= 120 && touchPoint.y <= 220) {
                  insertToGivenPNumber('2');
                } else if (touchPoint.x >= 120 && touchPoint.y >= 160 && touchPoint.x <= 185 && touchPoint.y <= 220) {
                  insertToGivenPNumber('3');
                } else if (touchPoint.x >= 185 && touchPoint.y >= 160 && touchPoint.x <= 240 && touchPoint.y <= 220) {
                  insertToGivenPNumber('4');
                } else if (touchPoint.x >= 0 && touchPoint.y >= 220 && touchPoint.x <= 75 && touchPoint.y <= 260) {
                  insertToGivenPNumber('5');
                } else if (touchPoint.x >= 75 && touchPoint.y >= 220 && touchPoint.x <= 120 && touchPoint.y <= 260) {
                  insertToGivenPNumber('6');
                } else if (touchPoint.x >= 120 && touchPoint.y >= 220 && touchPoint.x <= 186 && touchPoint.y <= 260) {
                  insertToGivenPNumber('7');
                } else if (touchPoint.x >= 185 && touchPoint.y >= 220 && touchPoint.x <= 240 && touchPoint.y <= 260) {
                  insertToGivenPNumber('8');
                } else if (touchPoint.x >= 0 && touchPoint.y >= 260 && touchPoint.x <= 75 && touchPoint.y <= 320) {
                  insertToGivenPNumber('9');
                } else if (touchPoint.x >= 75 && touchPoint.y >= 260 && touchPoint.x <= 120 && touchPoint.y <= 320) {
                  insertToGivenPNumber('0');
                } else if (touchPoint.x >= 120 && touchPoint.y >= 260 && touchPoint.x <= 240 && touchPoint.y <= 320) {
                  insertToGivenPNumber('M');
                } else if (touchPoint.x >= 20 && touchPoint.y >= 120 && touchPoint.x <= 220 && touchPoint.y <= 150) {
                  subExit = true;
                  willCall = true;
                }
                if (touchPoint.y >= 160) {
                  tft.setTextSize(3);
                  tft.setTextColor(black, white);
                  tft.setCursor(35, 75);
                  for (int i = 0; i < 9; i++) {
                    if (givenPNumber[i] != ' ') {
                      tft.print(givenPNumber[i]);
                      delay(10);
                    }
                  }
                }
              }
              while (ts.touched()) {}
            }
          }
          if (DEMO == false && willCall == true) {
            fona.callPhone(givenPNumber);
          }
          slidePage(true, white);
          givenPNumber[0] = ' ';
          givenPNumber[1] = ' ';
          givenPNumber[2] = ' ';
          givenPNumber[3] = ' ';
          givenPNumber[4] = ' ';
          givenPNumber[5] = ' ';
          givenPNumber[6] = ' ';
          givenPNumber[7] = ' ';
          givenPNumber[8] = ' ';
          tft.setTextSize(2);
          tft.setTextColor(black, white);
          tft.setCursor(20, 80);
          tft.print(F("Answer to call"));
          tft.drawFastHLine(0, 120, 240, darkgrey);
          tft.setCursor(20, 140);
          tft.print(F("End call"));
          tft.drawFastHLine(0, 180, 240, darkgrey);
          tft.setCursor(20, 200);
          tft.print(F("Call to number"));
        }
      }
    }
    updateTime();
    if (timeUpdated) {
      drawTime(1, red);
      timeUpdated = false;
    }
    if (millis() - idleTimer >= (idleTimeout - 7000)) {
      screenDimmed = true;
      dimmedBL = (bl / 2);
      backlight(dimmedBL);
      if (millis() - idleTimer >= idleTimeout) {
        lock();
        idleTimer = millis();
        screenDimmed = false;
      }
    }
  }
  exitApp();
}

void insertToGivenPNumber(char a) {
  if (a == 'M') {
    if (givenPNumber[8] != ' ') {
      givenPNumber[8] = ' ';
    } else if (givenPNumber[7] != ' ') {
      givenPNumber[7] = ' ';
    } else if (givenPNumber[6] != ' ') {
      givenPNumber[6] = ' ';
    } else if (givenPNumber[5] != ' ') {
      givenPNumber[5] = ' ';
    } else if (givenPNumber[4] != ' ') {
      givenPNumber[4] = ' ';
    } else if (givenPNumber[3] != ' ') {
      givenPNumber[3] = ' ';
    } else if (givenPNumber[2] != ' ') {
      givenPNumber[2] = ' ';
    } else if (givenPNumber[1] != ' ') {
      givenPNumber[1] = ' ';
    } else if (givenPNumber[0] != ' ') {
      givenPNumber[0] = ' ';
    }
    tft.fillRect(7, 62, 226, 46, white);
  } else {
    if (givenPNumber[0] == ' ') {
      givenPNumber[0] = a;
    } else if (givenPNumber[1] == ' ') {
      givenPNumber[1] = a;
    } else if (givenPNumber[2] == ' ') {
      givenPNumber[2] = a;
    } else if (givenPNumber[3] == ' ') {
      givenPNumber[3] = a;
    } else if (givenPNumber[4] == ' ') {
      givenPNumber[4] = a;
    } else if (givenPNumber[5] == ' ') {
      givenPNumber[5] = a;
    } else if (givenPNumber[6] == ' ') {
      givenPNumber[6] = a;
    } else if (givenPNumber[7] == ' ') {
      givenPNumber[7] = a;
    } else if (givenPNumber[8] == ' ') {
      givenPNumber[8] = a;
    }
  }
}

void messages() {
  draw(MESSAGES, 1);
  bool exit = false;
  TS_Point touchPoint = ts.getPoint();
  touchPoint.x = map(touchPoint.x, 0, 240, 240, 0);
  touchPoint.y = map(touchPoint.y, 0, 320, 320, 0);
  while (exit == false) {
    if (ts.touched()) {
      idleTimer = millis();
      if (screenDimmed) {
        backlight(bl);
        screenDimmed = false;
      } else {
        TS_Point touchPoint = ts.getPoint();
        touchPoint.x = map(touchPoint.x, 0, 240, 240, 0);
        touchPoint.y = map(touchPoint.y, 0, 320, 320, 0);
        if (touchPoint.y <= 50) {
          exit = true;
        }
      }
    }
    updateTime();
    if (timeUpdated) {
      drawTime(1, green);
      timeUpdated = false;
    }
    if (millis() - idleTimer >= (idleTimeout - 7000)) {
      screenDimmed = true;
      dimmedBL = (bl / 2);
      backlight(dimmedBL);
      if (millis() - idleTimer >= idleTimeout) {
        lock();
        idleTimer = millis();
        screenDimmed = false;
      }
    }
  }
  exitApp();
}

void contacts() {
  draw(CONTACTS, 1);
  bool exit = false;
  TS_Point touchPoint = ts.getPoint();
  touchPoint.x = map(touchPoint.x, 0, 240, 240, 0);
  touchPoint.y = map(touchPoint.y, 0, 320, 320, 0);
  while (exit == false) {
    if (ts.touched()) {
      idleTimer = millis();
      if (screenDimmed) {
        backlight(bl);
        screenDimmed = false;
      } else {
        TS_Point touchPoint = ts.getPoint();
        touchPoint.x = map(touchPoint.x, 0, 240, 240, 0);
        touchPoint.y = map(touchPoint.y, 0, 320, 320, 0);
        if (touchPoint.y <= 50) {
          exit = true;
        }
      }
    }
    updateTime();
    if (timeUpdated) {
      drawTime(1, blue);
      timeUpdated = false;
    }
    if (millis() - idleTimer >= (idleTimeout - 7000)) {
      screenDimmed = true;
      dimmedBL = (bl / 2);
      backlight(dimmedBL);
      if (millis() - idleTimer >= idleTimeout) {
        lock();
        idleTimer = millis();
        screenDimmed = false;
      }
    }
  }
  exitApp();
}

void pong() {
  draw(PONG, 1);
  bool exit = false;
  TS_Point touchPoint = ts.getPoint();
  touchPoint.x = map(touchPoint.x, 0, 240, 240, 0);
  touchPoint.y = map(touchPoint.y, 0, 320, 320, 0);
  while (exit == false) {
    if (ts.touched()) {
      idleTimer = millis();
      if (screenDimmed) {
        backlight(bl);
        screenDimmed = false;
      } else {
        TS_Point touchPoint = ts.getPoint();
        touchPoint.x = map(touchPoint.x, 0, 240, 240, 0);
        touchPoint.y = map(touchPoint.y, 0, 320, 320, 0);
        if (touchPoint.y <= 50) {
          exit = true;
        }
      }
    }
    if (millis() - idleTimer >= (idleTimeout - 7000)) {
      screenDimmed = true;
      dimmedBL = (bl / 2);
      backlight(dimmedBL);
      if (millis() - idleTimer >= idleTimeout) {
        lock();
        idleTimer = millis();
        screenDimmed = false;
      }
    }
  }
  exitApp();
}

void exitApp() {
  draw(MENU, 1);
}

/*void serialPrintCoordinates() {
  TS_Point touchPoint = ts.getPoint();
  touchPoint.x = map(touchPoint.x, 0, 240, 240, 0);
  touchPoint.y = map(touchPoint.y, 0, 320, 320, 0);
  Serial.print(F("X: "));
  Serial.print(touchPoint.x);
  Serial.print(F(", Y: "));
  Serial.println(touchPoint.y);
  }*/

bool checkNetStat() {
  netStat = fona.getNetworkStatus();
  if (netStat != oldNetStat) {
    return true;
  } else {
    return false;
  }
}

void printNetStat() {
  netStat = fona.getNetworkStatus();
  tft.setTextSize(1);
  tft.setTextColor(white, blue);
  tft.setCursor(5, 7);
  switch (netStat) {
    case 0:
    case 3:
    case 4:
      tft.print(F("No network"));
      break;
    case 1:
    case 5:
      tft.print(F("Registered"));
      break;
    case 2:
      tft.print(F("Searching"));
      break;
    default:
      tft.print(F("No network"));
      break;
  }
}






#include <Arduino.h>
#include <SoftwareSerial.h>
#include <DFRobotDFPlayerMini.h>
#include <U8g2lib.h>
#include <Wire.h>
#include <EEPROM.h>
#include "OneButton.h"

const uint8_t leftButtonPin = 2;
const uint8_t selectionButtonPin = 5;
const uint8_t rightButtonPin = 8;

uint8_t sMenuSelection = 2;
uint8_t selection = 1;
bool updateScreen = true;

// Variables
uint8_t filecounts, foldercounts;
uint8_t volume = 20, folder = 1, file = 1, eq = 0;

bool playing = false;
bool inSideMenuSelection = true;

OneButton PreviousBTN(leftButtonPin, true);
OneButton PlayBTN(selectionButtonPin, true);
OneButton NextBTN(rightButtonPin, true);

SoftwareSerial customSoftwareSerial(12, 13);
DFRobotDFPlayerMini myDFPlayer;

U8G2_SH1106_128X64_NONAME_1_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);

// Draw custom icons using basic shapes
void drawPlayIcon(uint8_t x, uint8_t y) {
  u8g2.drawTriangle(x, y, x, y+8, x+6, y+4);
}

void drawPauseIcon(uint8_t x, uint8_t y) {
  u8g2.drawBox(x, y, 2, 8);
  u8g2.drawBox(x+4, y, 2, 8);
}

void drawPrevIcon(uint8_t x, uint8_t y) {
  u8g2.drawBox(x+6, y, 2, 8);
  u8g2.drawTriangle(x+6, y, x+6, y+8, x, y+4);
}

void drawNextIcon(uint8_t x, uint8_t y) {
  u8g2.drawTriangle(x, y, x, y+8, x+6, y+4);
  u8g2.drawBox(x+6, y, 2, 8);
}

void drawSettingsIcon(uint8_t x, uint8_t y) {
  u8g2.drawCircle(x+6, y+6, 4);
  u8g2.drawBox(x+5, y+2, 2, 8);
  u8g2.drawBox(x+2, y+5, 8, 2);
}

void drawMusicIcon(uint8_t x, uint8_t y) {
  u8g2.drawBox(x, y+4, 2, 6);
  u8g2.drawBox(x+6, y+2, 2, 8);
  u8g2.drawCircle(x+1, y+10, 2);
  u8g2.drawCircle(x+7, y+10, 2);
  u8g2.drawLine(x+2, y+4, x+8, y+2);
}

void flashPage() {
  u8g2.setFont(u8g2_font_ncenB10_tr);
  u8g2.drawStr(15, 30, "MP3 Player");
  u8g2.setFont(u8g2_font_6x10_tr);
  u8g2.drawStr(10, 45, "By EDGE @ UCSD");
}

void setup(void) {
  Serial.begin(9600);
  Serial.println(F("Starting MP3 Player..."));
  
  u8g2.begin();
  u8g2.firstPage();
  do { flashPage(); } while (u8g2.nextPage());
  
  PreviousBTN.attachClick(preivousButtonClicked);
  PlayBTN.attachClick(playButtonClicked);
  NextBTN.attachClick(nextButtonClicked);

  PreviousBTN.setDebounceMs(50);
  PlayBTN.setDebounceMs(50);
  NextBTN.setDebounceMs(50);

  customSoftwareSerial.begin(9600);
  Serial.println(F("Initializing DFPlayer..."));
  
  delay(1500); // Give DFPlayer more time to initialize
  
  if (!myDFPlayer.begin(customSoftwareSerial)) {
    Serial.println(F("DFPlayer initialization FAILED!"));
    Serial.println(F("Check wiring:"));
    Serial.println(F("  DFPlayer RX -> Arduino Pin 13"));
    Serial.println(F("  DFPlayer TX -> Arduino Pin 12"));
    Serial.println(F("Continuing anyway..."));
  } else {
    Serial.println(F("DFPlayer initialized successfully!"));
  }
  
  volume = EEPROM.read(0);
  if (volume > 30) volume = 20; // Default to 20 if invalid
  eq = EEPROM.read(1);
  if (eq > 5) eq = 0;
  file = EEPROM.read(2);
  if (file >= 255 || file == 0) file = 1;

  delay(500);
  myDFPlayer.volume(volume);
  Serial.print(F("Volume set to: "));
  Serial.println(volume);
  
  delay(500);
  
  // Check SD card status
  int state = myDFPlayer.readState();
  Serial.print(F("DFPlayer state: "));
  Serial.println(state);
  
  foldercounts = myDFPlayer.readFolderCounts();
  Serial.print(F("Total folders found: "));
  Serial.println(foldercounts);
  
  startFolderPlay();
  
  Serial.println(F("Setup complete!"));
  Serial.println(F("Waiting for commands..."));
}

void loop() {
  PreviousBTN.tick();
  PlayBTN.tick();
  NextBTN.tick();
  updateDisplay();
  updateDFplayer();
}

void preivousButtonClicked() {
  updateScreen = true;
  // In side menu: move up (Settings to Player)
  if (inSideMenuSelection && sMenuSelection > 1) {
    sMenuSelection--;
  }
  // In Player screen: move LEFT through options (4→3→2→1, wrapping)
  else if (sMenuSelection == 1 && !inSideMenuSelection) {
    if (selection > 1) selection--;
    else selection = 4; // Wrap from 1 to 4
  }
  // In Settings screen when adjusting: decrease value
  else if (selection == 1 && volume > 0 && sMenuSelection == 2 && !inSideMenuSelection) {
    volume--;
  }
  else if (selection == 2 && eq > 0 && sMenuSelection == 2 && !inSideMenuSelection) {
    eq--;
  }
}

void nextButtonClicked() {
  updateScreen = true;
  // In side menu: move down (Player to Settings)
  if (inSideMenuSelection && sMenuSelection < 2) {
    sMenuSelection++;
  }
  // In Player screen: move RIGHT through options (1→2→3→4, wrapping)
  else if (sMenuSelection == 1 && !inSideMenuSelection) {
    if (selection < 4) selection++;
    else selection = 1; // Wrap from 4 to 1
  }
  // In Settings screen when adjusting: increase value
  else if (selection == 1 && volume < 30 && sMenuSelection == 2 && !inSideMenuSelection) {
    volume++;
  }
  else if (selection == 2 && eq < 5 && sMenuSelection == 2 && !inSideMenuSelection) {
    eq++;
  }
}

void playButtonClicked() {
  if (inSideMenuSelection) {
    inSideMenuSelection = false;
    updateScreen = true;
    delay(100);
  } else if (!inSideMenuSelection && sMenuSelection == 1) {
    if (selection == 1) {
      if (file > 1) {
        file--;
        Serial.print(F("Previous track: "));
        Serial.println(file);
        myDFPlayer.previous();
        if (!playing) playing = true;
        EEPROM.write(2, file);
      }
    } else if (selection == 2) {
      if (playing) {
        Serial.println(F("Pausing..."));
        myDFPlayer.pause();
      } else {
        Serial.println(F("Playing..."));
        myDFPlayer.start();
      }
      playing = !playing;
    } else if (selection == 3) {
      file++;
      Serial.print(F("Next track: "));
      Serial.println(file);
      myDFPlayer.next();
      if (!playing) playing = true;
      EEPROM.write(2, file);
    } else if (selection == 4) {
      selection = 1;
      inSideMenuSelection = true;
    }
    updateScreen = true;
    delay(200);
  } else if (!inSideMenuSelection && sMenuSelection == 2) {
    if (selection == 1) {
      selection = 2;
      Serial.print(F("Volume changed to: "));
      Serial.println(volume);
      myDFPlayer.volume(volume);
      EEPROM.write(0, volume);
    } else if (selection == 2) {
      selection = 4;
      Serial.print(F("EQ changed to: "));
      Serial.println(eq);
      myDFPlayer.EQ(eq);
      EEPROM.write(1, eq);
    } else if (selection == 4) {
      selection = 1;
      inSideMenuSelection = true;
    }
    updateScreen = true;
    delay(200);
  }
}

void startFolderPlay() {
  delay(200); // Give more time for SD card reading
  filecounts = myDFPlayer.readFileCountsInFolder(folder);
  
  Serial.print(F("Files in folder "));
  Serial.print(folder);
  Serial.print(F(": "));
  Serial.println(filecounts);
  
  // 255 often means communication error or no SD card
  if (filecounts == 255) {
    Serial.println(F("ERROR: SD card not detected or no files found!"));
    Serial.println(F("Check:"));
    Serial.println(F("  1. SD card inserted properly"));
    Serial.println(F("  2. Files in /01/ folder"));
    Serial.println(F("  3. Files named 001.mp3, 002.mp3, etc."));
    Serial.println(F("  4. SD card formatted as FAT32"));
    filecounts = 1; // Assume at least 1 file to avoid errors
  }
  
  if (file > filecounts) file = 1; // Reset if file number too high
  
  Serial.print(F("Playing folder "));
  Serial.print(folder);
  Serial.print(F(", file "));
  Serial.println(file);
  
  myDFPlayer.playFolder(folder, file);
  delay(100);
  playing = true; // Set to true since we just started playing
}

void updateDFplayer() {
  if (myDFPlayer.available()) {
    uint8_t type = myDFPlayer.readType();
    switch (type) {
      case DFPlayerPlayFinished:
        if (file < filecounts) {
          file++;
          myDFPlayer.playFolder(folder, file);
          EEPROM.write(2, file);
          updateScreen = true;
        }
        break;
    }
  }
}

void updateDisplay() {
  if (updateScreen) {
    u8g2.firstPage();
    do {
      if (sMenuSelection == 1) player();
      else if (sMenuSelection == 2) settings();
    } while (u8g2.nextPage());
    updateScreen = false;
  }
}

void settings() {
  sideMenu();
  topMenu();
  u8g2.setFont(u8g2_font_7x13_tr);
  u8g2.drawStr(40, 17, "Settings");
  
  u8g2.setFont(u8g2_font_6x10_tr);
  
  // Volume option
  if (selection == 1 && !inSideMenuSelection) {
    u8g2.drawRFrame(28, 26, 90, 16, 2);
  }
  u8g2.drawStr(30, 38, "Volume:");
  if (selection == 1 && !inSideMenuSelection) {
    if (volume > 0) {
      u8g2.drawTriangle(85, 35, 85, 31, 82, 33);
    }
    if (volume < 30) {
      u8g2.drawTriangle(110, 31, 110, 35, 113, 33);
    }
  }
  u8g2.setCursor(92, 38);
  u8g2.print(volume);
  
  // EQ option
  if (selection == 2 && !inSideMenuSelection) {
    u8g2.drawRFrame(28, 43, 90, 16, 2);
  }
  u8g2.drawStr(30, 55, "EQ:");
  if (selection == 2 && !inSideMenuSelection) {
    if (eq > 0) {
      u8g2.drawTriangle(85, 52, 85, 48, 82, 50);
    }
    if (eq < 5) {
      u8g2.drawTriangle(110, 48, 110, 52, 113, 50);
    }
  }
  u8g2.setCursor(92, 55);
  u8g2.print(eq);
  
  // OK button
  if (selection == 4 && !inSideMenuSelection) {
    u8g2.drawRFrame(102, 52, 22, 12, 2);
  }
  u8g2.drawStr(106, 62, "OK");
}

void topMenu() {
  u8g2.setFont(u8g2_font_6x10_tr);
  u8g2.drawStr(100, 10, "V:");
  u8g2.setCursor(112, 10);
  u8g2.print(volume);
}

void sideMenu() {
  u8g2.setDrawColor(1);
  
  // Player icon (music note)
  if (sMenuSelection == 1 && inSideMenuSelection) {
    u8g2.drawRFrame(1, 10, 22, 22, 3);
  }
  drawMusicIcon(6, 12);
  
  // Settings icon (gear)
  if (sMenuSelection == 2 && inSideMenuSelection) {
    u8g2.drawRFrame(1, 34, 22, 22, 3);
  }
  drawSettingsIcon(6, 36);
  
  u8g2.drawLine(25, 0, 25, 64);
}

void player() {
  sideMenu();
  topMenu();
  
  u8g2.setFont(u8g2_font_6x10_tr);
  u8g2.drawStr(30, 20, "Track:");
  u8g2.setCursor(70, 20);
  u8g2.print(file);
  u8g2.drawStr(80, 20, "/");
  u8g2.setCursor(90, 20);
  u8g2.print(filecounts);
  
  uint8_t baseX = 45;
  uint8_t baseY = 40;
  
  // Previous button
  if (selection == 1 && !inSideMenuSelection) {
    u8g2.drawRFrame(baseX-3, baseY-3, 14, 14, 2);
  }
  drawPrevIcon(baseX, baseY);
  
  // Play/Pause button
  if (selection == 2 && !inSideMenuSelection) {
    u8g2.drawRFrame(baseX+17, baseY-3, 14, 14, 2);
  }
  if (playing)
    drawPauseIcon(baseX+20, baseY);
  else
    drawPlayIcon(baseX+20, baseY);
  
  // Next button
  if (selection == 3 && !inSideMenuSelection) {
    u8g2.drawRFrame(baseX+37, baseY-3, 14, 14, 2);
  }
  drawNextIcon(baseX+40, baseY);
  
  // Back button
  if (selection == 4 && !inSideMenuSelection) {
    u8g2.drawRFrame(102, 52, 22, 12, 2);
  }
  u8g2.setFont(u8g2_font_6x10_tr);
  u8g2.drawStr(106, 62, "OK");
}
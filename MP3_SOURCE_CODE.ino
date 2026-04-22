// ===== LIBRARIES =====
// These give us extra functionality (like tools we import)
#include <Arduino.h>
#include <SoftwareSerial.h>      // Allows serial communication on other pins
#include <DFRobotDFPlayerMini.h> // Controls the DFPlayer Mini MP3 module
#include <U8g2lib.h>             // Controls the OLED display
#include <Wire.h>
#include <EEPROM.h>              // Stores values even after power is turned off
#include "OneButton.h"           // Makes button click handling easier

// ===== PIN SETUP ===== (These are the Arduino pins connected to the 3 buttons)
const uint8_t leftButtonPin = ;      // === FIX ME===
const uint8_t selectionButtonPin = ; // === FIX ME ===
const uint8_t rightButtonPin = ;     // === FIX ME ===

// ===== MENU VARIABLES =====

// sMenuSelection controls which main screen we are on
// 1 = Player screen
// 2 = Settings screen
uint8_t sMenuSelection = 2;

// selection controls which option inside a screen is highlighted
uint8_t selection = 1;

// updateScreen tells the OLED when it needs to redraw
bool updateScreen = true;

// ===== MP3 PLAYER VARIABLES =====
uint8_t filecounts, foldercounts;
uint8_t volume = 20, folder = 1, file = 1, eq = 0;

// ===== STATE VARIABLES =====
bool playing = false;             // true if music is currently playing
bool inSideMenuSelection = true;  // true if selecting between Player/Settings icons

// ===== BUTTON OBJECTS =====
// true means the buttons are active LOW
OneButton PreviousBTN(leftButtonPin, true);
OneButton PlayBTN(selectionButtonPin, true);
OneButton NextBTN(rightButtonPin, true);

// ===== SERIAL + MP3 PLAYER =====
SoftwareSerial customSoftwareSerial(12, 13); // RX, TX
DFRobotDFPlayerMini myDFPlayer;

// ===== DISPLAY OBJECT =====
U8G2_SH1106_128X64_NONAME_1_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);

// ======================================================
// DRAW FUNCTIONS
// ======================================================

// triangle play icon
void drawPlayIcon(uint8_t x, uint8_t y) {
  u8g2.drawTriangle(x, y, x, y + 8, x + 6, y + 4);
}

// pause icon using two small rectangles
void drawPauseIcon(uint8_t x, uint8_t y) {
  u8g2.drawBox(x, y, 2, 8);
  u8g2.drawBox(x + 4, y, 2, 8);
}

// previous-track icon
void drawPrevIcon(uint8_t x, uint8_t y) {
  u8g2.drawBox(x + 6, y, 2, 8);
  u8g2.drawTriangle(x + 6, y, x + 6, y + 8, x, y + 4);
}

// next-track icon
void drawNextIcon(uint8_t x, uint8_t y) {
  u8g2.drawTriangle(x, y, x, y + 8, x + 6, y + 4);
  u8g2.drawBox(x + 6, y, 2, 8);
}

// settings/gear-like icon
void drawSettingsIcon(uint8_t x, uint8_t y) {
  u8g2.drawCircle(x + 6, y + 6, 4);
  u8g2.drawBox(x + 5, y + 2, 2, 8);
  u8g2.drawBox(x + 2, y + 5, 8, 2);
}

// music note icon
void drawMusicIcon(uint8_t x, uint8_t y) {
  u8g2.drawBox(x, y + 4, 2, 6);
  u8g2.drawBox(x + 6, y + 2, 2, 8);
  u8g2.drawCircle(x + 1, y + 10, 2);
  u8g2.drawCircle(x + 7, y + 10, 2);
  u8g2.drawLine(x + 2, y + 4, x + 8, y + 2);
}

// ===== SPLASH SCREEN =====
// startup page 
void flashPage() {
  u8g2.setFont(u8g2_font_ncenB10_tr);
  u8g2.drawStr(15, 30, "MP3 Player");
  u8g2.setFont(u8g2_font_6x10_tr);
  u8g2.drawStr(10, 45, "By _______");     //====FIX ME!!==== (add your team's name!)
}

// ======================================================
// SETUP
// ======================================================
void setup(void) {
  Serial.begin(9600);
  Serial.println(F("Starting MP3 Player..."));

  // Start the OLED display
  u8g2.begin();

  // Show splash screen once
  u8g2.firstPage();
  do {
    flashPage();
  } while (u8g2.nextPage());

  // Attach button click functions
  PreviousBTN.attachClick(previousButtonClicked);
  PlayBTN.attachClick(playButtonClicked);
  NextBTN.attachClick(nextButtonClicked);

  // Reduce button bouncing/noise
  PreviousBTN.setDebounceMs(50);
  PlayBTN.setDebounceMs(50);
  NextBTN.setDebounceMs(50);

  // Start serial connection to DFPlayer
  customSoftwareSerial.begin(9600);
  Serial.println(F("Initializing DFPlayer..."));

  delay(1500); // Give DFPlayer time to start up

  // Try to initialize DFPlayer 
  if (!myDFPlayer.begin(customSoftwareSerial)) {
    Serial.println(F("DFPlayer initialization FAILED!"));
    Serial.println(F("Check wiring:"));
    Serial.println(F("  DFPlayer RX -> Arduino Pin 13"));
    Serial.println(F("  DFPlayer TX -> Arduino Pin 12"));
    Serial.println(F("Continuing anyway..."));
  } else {
    Serial.println(F("DFPlayer initialized successfully!"));
  }

  // Load saved settings from EEPROM memory
  volume = EEPROM.read(0);
  if (volume > 30) volume = 20; // If bad value is stored, use default

  eq = EEPROM.read(1);
  if (eq > 5) eq = 0;

  file = EEPROM.read(2);
  if (file >= 255 || file == 0) file = 1;

  delay(500);
  myDFPlayer.volume(volume);

  Serial.print(F("Volume set to: "));
  Serial.println(volume);

  delay(500);

  // Check DFPlayer state
  int state = myDFPlayer.readState();
  Serial.print(F("DFPlayer state: "));
  Serial.println(state);

  // Read number of folders on the SD card
  foldercounts = myDFPlayer.readFolderCounts();
  Serial.print(F("Total folders found: "));
  Serial.println(foldercounts);

  // Start playing the current file in the current folder
  startFolderPlay();

  Serial.println(F("Setup complete!"));
  Serial.println(F("Waiting for commands..."));
}

// ======================================================
// LOOP
// ======================================================
void loop() {
  // Check each button to see if it was pressed
  PreviousBTN.tick();
  PlayBTN.tick();
  NextBTN.tick();

  // Update the screen only when needed
  updateDisplay();

  // Check DFPlayer for song-finished events
  updateDFplayer();
}

// ======================================================
// BUTTON FUNCTIONS
// ======================================================

void previousButtonClicked() {
  updateScreen = true;

  // if we are in the side menu, move up
  if (inSideMenuSelection && sMenuSelection > 1) {
    sMenuSelection--;
  }
  // If we are in the Player screen, move selection left
  else if (sMenuSelection == 1 && !inSideMenuSelection) {
    if (selection > 1) selection--;
    else selection = 4; // wrap around from 1 back to 4
  }
  // If we are in Settings and adjusting volume, decrease volume
  else if (selection == 1 && volume > 0 && sMenuSelection == 2 && !inSideMenuSelection) {
    volume--;
  }
  // If we are in Settings and adjusting EQ, decrease EQ
  else if (selection == 2 && eq > 0 && sMenuSelection == 2 && !inSideMenuSelection) {
    eq--;
  }
}

void nextButtonClicked() {
  updateScreen = true;

  // If we are in the side menu, move down
  if (inSideMenuSelection && sMenuSelection < 2) {
    sMenuSelection++;
  }
  // If we are in the Player screen, move selection right
  else if (sMenuSelection == 1 && !inSideMenuSelection) {
    if (selection < 4) selection++;
    else selection = 1; // wrap around from 4 back to 1
  }
  // If we are in Settings and adjusting volume, increase volume
  else if (selection == 1 && volume < 30 && sMenuSelection == 2 && !inSideMenuSelection) {
    volume++;
  }
  // If we are in Settings and adjusting EQ, increase EQ
  else if (selection == 2 && eq < 5 && sMenuSelection == 2 && !inSideMenuSelection) {
    eq++;
  }
}

void playButtonClicked() {
  // If we are currently selecting the side menu,
  // pressing the play/select button enters that screen
  if (inSideMenuSelection) {
    inSideMenuSelection = false;
    updateScreen = true;
    delay(100);
  }

  // ===== PLAYER SCREEN BUTTON ACTIONS =====
  else if (!inSideMenuSelection && sMenuSelection == 1) {
    // Previous track
    if (selection == 1) {
      if (file > 1) {
        file--;
        Serial.print(F("Previous track: "));
        Serial.println(file);
        myDFPlayer.previous();
        if (!playing) playing = true;
        EEPROM.write(2, file);
      }
    }

    // Play / Pause
    else if (selection == 2) {
      if (playing) {
        Serial.println(F("Pausing..."));
        myDFPlayer.pause();
      } else {
        Serial.println(F("Playing..."));
        myDFPlayer.start();
      }
      playing = !playing;
    }

    // Next track
    else if (selection == 3) {
      file++;
      Serial.print(F("Next track: "));
      Serial.println(file);
      myDFPlayer.next();
      if (!playing) playing = true;
      EEPROM.write(2, file);
    }

    // OK / Back
    else if (selection == 4) {
      selection = 1;
      inSideMenuSelection = true;
    }

    updateScreen = true;
    delay(200);
  }

  // ===== SETTINGS SCREEN BUTTON ACTIONS =====
  else if (!inSideMenuSelection && sMenuSelection == 2) {
    // Save volume
    if (selection == 1) {
      selection = 2;
      Serial.print(F("Volume changed to: "));
      Serial.println(volume);
      myDFPlayer.volume(volume);
      EEPROM.write(0, volume);
    }

    // Save EQ
    else if (selection == 2) {
      selection = 4;
      Serial.print(F("EQ changed to: "));
      Serial.println(eq);
      myDFPlayer.EQ(eq);
      EEPROM.write(1, eq);
    }

    // OK / Back
    else if (selection == 4) {
      selection = 1;
      inSideMenuSelection = true;
    }

    updateScreen = true;
    delay(200);
  }
}

// ======================================================
// START PLAYBACK
// Reads SD card info and starts music
// ======================================================
void startFolderPlay() {
  delay(200); // Give SD card/DFPlayer some time
  filecounts = myDFPlayer.readFileCountsInFolder(folder);

  Serial.print(F("Files in folder "));
  Serial.print(folder);
  Serial.print(F(": "));
  Serial.println(filecounts);

  // 255 usually means an error or missing SD card
  if (filecounts == 255) {
    Serial.println(F("ERROR: SD card not detected or no files found!"));
    Serial.println(F("Check:"));
    Serial.println(F("  1. SD card inserted properly"));
    Serial.println(F("  2. Files in /01/ folder"));
    Serial.println(F("  3. Files named 001.mp3, 002.mp3, etc."));
    Serial.println(F("  4. SD card formatted as FAT32"));
    filecounts = 1; // fallback so code does not break
  }

  // If saved file number is larger than what exists, reset to file 1
  if (file > filecounts) file = 1;

  Serial.print(F("Playing folder "));
  Serial.print(folder);
  Serial.print(F(", file "));
  Serial.println(file);

  myDFPlayer.playFolder(folder, file);
  delay(100);
  playing = true;
}

// ======================================================
// DFPLAYER UPDATE
// ======================================================
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

// ======================================================
// DISPLAY UPDATE
// ======================================================
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

// ======================================================
// SETTINGS SCREEN
// ======================================================
void settings() {
  sideMenu();
  topMenu();

  u8g2.setFont(u8g2_font_7x13_tr);
  u8g2.drawStr(40, 17, "Settings");

  u8g2.setFont(u8g2_font_6x10_tr);

  // ===== Volume option =====
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

  // ===== EQ option =====
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

  // ===== OK button =====
  if (selection == 4 && !inSideMenuSelection) {
    u8g2.drawRFrame(102, 52, 22, 12, 2);
  }
  u8g2.drawStr(106, 62, "OK");
}

// ======================================================
// TOP MENU BAR
// Shows volume at the top of the screen
// ======================================================
void topMenu() {
  u8g2.setFont(u8g2_font_6x10_tr);
  u8g2.drawStr(100, 10, "V:");
  u8g2.setCursor(112, 10);
  u8g2.print(volume);
}

// ======================================================
// SIDE MENU
// Draws icons for Player and Settings
// ======================================================
void sideMenu() {
  u8g2.setDrawColor(1);

  // Highlight Player icon if selected
  if (sMenuSelection == 1 && inSideMenuSelection) {
    u8g2.drawRFrame(1, 10, 22, 22, 3);
  }
  drawMusicIcon(6, 12);

  // Highlight Settings icon if selected
  if (sMenuSelection == 2 && inSideMenuSelection) {
    u8g2.drawRFrame(1, 34, 22, 22, 3);
  }
  drawSettingsIcon(6, 36);

  // Divider line between side menu and main content
  u8g2.drawLine(25, 0, 25, 64);
}

// ======================================================
// PLAYER SCREEN
// Draws track info and control icons
// ======================================================
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

  // ===== Previous button =====
  if (selection == 1 && !inSideMenuSelection) {
    u8g2.drawRFrame(baseX - 3, baseY - 3, 14, 14, 2);
  }
  drawPrevIcon(baseX, baseY);

  // ===== Play/Pause button =====
  if (selection == 2 && !inSideMenuSelection) {
    u8g2.drawRFrame(baseX + 17, baseY - 3, 14, 14, 2);
  }

  if (playing)
    drawPauseIcon(baseX + 20, baseY);
  else
    drawPlayIcon(baseX + 20, baseY);

  // ===== Next button =====
  if (selection == 3 && !inSideMenuSelection) {
    u8g2.drawRFrame(baseX + 37, baseY - 3, 14, 14, 2);
  }
  drawNextIcon(baseX + 40, baseY);

  // ===== Back / OK button =====
  if (selection == 4 && !inSideMenuSelection) {
    u8g2.drawRFrame(102, 52, 22, 12, 2);
  }
  u8g2.setFont(u8g2_font_6x10_tr);
  u8g2.drawStr(106, 62, "OK");
}

// ======================================================
// EXTRA PRACTICE 
// ======================================================

// TRY THIS:
// Change the starting volume:
// uint8_t volume = 10;

// TRY THIS:
// Change the default screen:
// uint8_t sMenuSelection = 1;

// TRY THIS:
// Add a custom message in player():
// u8g2.drawStr(30, 30, "Hello!");

// TRY THIS:
// Add another icon somewhere on the screen

// ======================================================
// END OF PROGRAM
// ======================================================

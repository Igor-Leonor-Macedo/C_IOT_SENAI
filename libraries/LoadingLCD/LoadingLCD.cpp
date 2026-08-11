#include "LoadingLCD.h"

LoadingLCD::LoadingLCD(LiquidCrystal_I2C &lcdRef) : lcd(lcdRef) {}

void LoadingLCD::loadStage(byte *char1, byte *char2, byte *char3, byte *char4) {
    lcd.createChar(0, char1);
    lcd.createChar(1, char2);
    lcd.createChar(4, char3);
    lcd.createChar(5, char4);
    lcd.setCursor(0, 0);
    lcd.write(byte(0));
    lcd.write(byte(1));
    lcd.setCursor(0, 1);
    lcd.write(byte(4));
    lcd.write(byte(5));
    delay(500);
}

void LoadingLCD::showLoading() {
    byte stage_A1[8] = {0b11111,0b11111,0b11111,0b11111,0b01111,0b00111,0b00011,0b00001};
    byte stage_A2[8] = {0b11111,0b11111,0b11111,0b11111,0b11110,0b11100,0b11000,0b10000};
    byte stage_A3[8] = {0b00001,0b00010,0b00100,0b01000,0b10000,0b10000,0b10000,0b11111};
    byte stage_A4[8] = {0b10000,0b01000,0b00100,0b00010,0b00001,0b00001,0b00001,0b11111};

    byte stage_B1[8] = {0b11111,0b10000,0b11111,0b11111,0b01111,0b00111,0b00011,0b00001};
    byte stage_B2[8] = {0b11111,0b00001,0b11111,0b11111,0b11110,0b11100,0b11000,0b10000};
    byte stage_B3[8] = {0b00001,0b00010,0b00100,0b01000,0b10000,0b10000,0b11111,0b11111};
    byte stage_B4[8] = {0b10000,0b01000,0b00100,0b00010,0b00001,0b00001,0b11111,0b11111};

    byte stage_C1[8] = {0b11111,0b10000,0b10000,0b11111,0b01111,0b00111,0b00011,0b00001};
    byte stage_C2[8] = {0b11111,0b00001,0b00001,0b11111,0b11110,0b11100,0b11000,0b10000};
    byte stage_C3[8] = {0b00001,0b00010,0b00100,0b01000,0b10000,0b11111,0b11111,0b11111};
    byte stage_C4[8] = {0b10000,0b01000,0b00100,0b00010,0b00001,0b11111,0b11111,0b11111};

    byte stage_D1[8] = {0b11111,0b10000,0b10000,0b10000,0b01111,0b00111,0b00011,0b00001};
    byte stage_D2[8] = {0b11111,0b00001,0b00001,0b00001,0b11110,0b11100,0b11000,0b10000};
    byte stage_D3[8] = {0b00001,0b00010,0b00100,0b01000,0b11111,0b11111,0b11111,0b11111};
    byte stage_D4[8] = {0b10000,0b01000,0b00100,0b00010,0b11111,0b11111,0b11111,0b11111};

    byte stage_E1[8] = {0b11111,0b10000,0b10000,0b10000,0b01000,0b00111,0b00011,0b00001};
    byte stage_E2[8] = {0b11111,0b00001,0b00001,0b00001,0b00010,0b11100,0b11000,0b10000};
    byte stage_E3[8] = {0b00001,0b00010,0b00100,0b01111,0b11111,0b11111,0b11111,0b11111};
    byte stage_E4[8] = {0b10000,0b01000,0b00100,0b11110,0b11111,0b11111,0b11111,0b11111};

    byte stage_F1[8] = {0b11111,0b10000,0b10000,0b10000,0b01000,0b00100,0b00011,0b00001};
    byte stage_F2[8] = {0b11111,0b00001,0b00001,0b00001,0b00010,0b00100,0b11000,0b10000};
    byte stage_F3[8] = {0b00001,0b00010,0b00111,0b01111,0b11111,0b11111,0b11111,0b11111};
    byte stage_F4[8] = {0b10000,0b01000,0b11100,0b11110,0b11111,0b11111,0b11111,0b11111};

    byte stage_G1[8] = {0b11111,0b10000,0b10000,0b10000,0b01000,0b00100,0b00010,0b00001};
    byte stage_G2[8] = {0b11111,0b00001,0b00001,0b00001,0b00010,0b00100,0b01000,0b10000};
    byte stage_G3[8] = {0b00001,0b00011,0b00111,0b01111,0b11111,0b11111,0b11111,0b11111};
    byte stage_G4[8] = {0b10000,0b11000,0b11100,0b11110,0b11111,0b11111,0b11111,0b11111};

    lcd.setCursor(3, 1);
    lcd.print("..LOADING...");
    loadStage(stage_A1, stage_A2, stage_A3, stage_A4);
    loadStage(stage_B1, stage_B2, stage_B3, stage_B4);
    loadStage(stage_C1, stage_C2, stage_C3, stage_C4);
    loadStage(stage_D1, stage_D2, stage_D3, stage_D4);
    loadStage(stage_E1, stage_E2, stage_E3, stage_E4);
    loadStage(stage_F1, stage_F2, stage_F3, stage_F4);
    loadStage(stage_G1, stage_G2, stage_G3, stage_G4);
}
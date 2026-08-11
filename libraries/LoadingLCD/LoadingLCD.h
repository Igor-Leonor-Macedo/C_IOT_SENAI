#ifndef LOADING_LCD_H
#define LOADING_LCD_H

#include <Arduino.h>
#include <LiquidCrystal_I2C.h>

class LoadingLCD {
public:
    LoadingLCD(LiquidCrystal_I2C &lcd);
    void showLoading();

private:
    LiquidCrystal_I2C &lcd;
    void loadStage(byte *char1, byte *char2, byte *char3, byte *char4);
};

#endif

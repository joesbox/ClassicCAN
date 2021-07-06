/*
    OBDII CAN Interface Display
    Using source code supplied with the Waveshare 1.28" circular LCD.

    © 2021 Joe Mann. https://wiki.joeblogs.co

    Version history:
    2021-07-04        v1.0.0        Initial release
*/

#include <CAN.h>
#include <OBD2.h>
#include <elapsedMillis.h>
#include <TimeLib.h>
#include <font_Arial.h>
#include <SPI.h>
#include "LCD_Driver.h"
#include "GUI_Paint.h"
#include "image.h"

int counter = 0;
bool countdown = false;
bool cleardigits = false;
bool OBDconnected = false;

elapsedMillis updateTimer;
uint32_t updateInt = 10000;

int oilPressure, oilTemp, waterTemp, RPM, maniPress, ignAdv;
int xPos;
int prevVal, currentVal;
int lcdBL;

bool switchDown = false;

PAINT_TIME currentTime;

enum Displays
{
  oilpressure,
  oiltemp,
  watertemp,
  rpm,
  manipressure,
  ignadv
};

Displays currentDisplay = oiltemp;

void setup()
{
  // Start OBD connection
  if (OBD2.begin())
  {
    OBDconnected = true;
  }
  OBDconnected = true;
  // Initialise IO
  Config_Init();

  // Sync time
  while (!Serial && millis() < 4000)
    ;
  Serial.println("\n" __FILE__ " " __DATE__ " " __TIME__);
  setSyncProvider(getTeensy3Time);

  // Initialise LCD
  LCD_Init();
  LCD_SetBacklight(lcdBL);
  Paint_NewImage(LCD_WIDTH, LCD_HEIGHT, 0, BLACK);

  // Draw graphics
  drawGFX();

  // Fade LCD on
  for (int lcdBL = 5; lcdBL < 255; lcdBL += 10)
  {
    LCD_SetBacklight(lcdBL);
  }
}

void loop()
{
  // Set time
  currentTime.Year = year();
  currentTime.Month = month();
  currentTime.Day = day();
  currentTime.Hour = hour();
  currentTime.Min = minute();
  currentTime.Sec = second();

  if (!switchDown)
  {
    RPM += 100;
    if (RPM == 6000)
    {
      switchDown = true;
    }
  }
  else
  {
    RPM -= 100;
    if (RPM == 0)
    {
      switchDown = false;
    }
  }

  updateGFX();
}

/**
 * Draws display graphics
 * */
void drawGFX()
{
  switch (currentDisplay)
  {
  case oilpressure:
    LCD_Clear(BLACK);
    LCD_ClearWindow(0, 0, 240, 32, GRAY);
    LCD_ClearWindow(0, 118, 240, 150, DARKGREEN);
    LCD_ClearWindow(0, 150, 240, 240, LIGHTGREEN);
    Paint_DrawString_EN(89, 119, "PSI", &FontAvenirNext20, DARKGREEN, WHITE);
    Paint_DrawString_EN(78, 151, "OIL", &FontBebasNeue40, LIGHTGREEN, WHITE);
    Paint_DrawString_EN(64, 190, "PRESSURE", &FontBebasNeue20, LIGHTGREEN, WHITE);
    if (OBDconnected)
    {
      Paint_DrawBitmap(112, 220, urlbmp_16x16, 16, 16, DARKGREEN, LIGHTGREEN);
    }
    break;

  case oiltemp:
    LCD_Clear(BLACK);
    LCD_ClearWindow(0, 0, 240, 32, GRAY);
    LCD_ClearWindow(0, 118, 240, 150, DARKGREEN);
    LCD_ClearWindow(0, 150, 240, 240, LIGHTGREEN);
    Paint_DrawString_EN(46, 119, "CELCIUS", &FontAvenirNext20, DARKGREEN, WHITE);
    Paint_DrawString_EN(78, 151, "OIL", &FontBebasNeue40, LIGHTGREEN, WHITE);
    Paint_DrawString_EN(91, 190, "TEMP", &FontBebasNeue20, LIGHTGREEN, WHITE);
    if (OBDconnected)
    {
      Paint_DrawBitmap(112, 220, urlbmp_16x16, 16, 16, DARKGREEN, LIGHTGREEN);
    }
    break;

  case watertemp:
    LCD_Clear(BLACK);
    LCD_ClearWindow(0, 0, 240, 32, GRAY);
    LCD_ClearWindow(0, 118, 240, 150, DARKBLUE);
    LCD_ClearWindow(0, 150, 240, 240, LIGHTBLUE);
    Paint_DrawString_EN(46, 119, "CELCIUS", &FontAvenirNext20, DARKBLUE, WHITE);
    Paint_DrawString_EN(50, 151, "WATER", &FontBebasNeue40, LIGHTBLUE, WHITE);
    Paint_DrawString_EN(91, 190, "TEMP", &FontBebasNeue20, LIGHTBLUE, WHITE);
    if (OBDconnected)
    {
      Paint_DrawBitmap(112, 220, urlbmp_16x16, 16, 16, DARKBLUE, LIGHTBLUE);
    }
    break;

  case rpm:
    LCD_Clear(BLACK);
    LCD_ClearWindow(0, 0, 240, 32, GRAY);
    LCD_ClearWindow(0, 118, 240, 150, DARKRED);
    LCD_ClearWindow(0, 150, 240, 240, RED);
    Paint_DrawString_EN(88, 119, "RPM", &FontAvenirNext20, DARKRED, WHITE);
    Paint_DrawString_EN(36, 151, "ENGINE", &FontBebasNeue40, RED, WHITE);
    Paint_DrawString_EN(85, 190, "SPEED", &FontBebasNeue20, RED, WHITE);
    if (OBDconnected)
    {
      Paint_DrawBitmap(112, 220, urlbmp_16x16, 16, 16, DARKRED, RED);
    }
    break;

  case manipressure:
    LCD_Clear(BLACK);
    LCD_ClearWindow(0, 0, 240, 32, GRAY);
    LCD_ClearWindow(0, 118, 240, 150, DMBLUE);
    LCD_ClearWindow(0, 150, 240, 240, MBLUE);
    Paint_DrawString_EN(88, 119, "kPa", &FontAvenirNext20, DMBLUE, WHITE);
    Paint_DrawString_EN(50, 151, "INLET", &FontBebasNeue40, MBLUE, WHITE);
    Paint_DrawString_EN(64, 190, "PRESSURE", &FontBebasNeue20, MBLUE, WHITE);
    if (OBDconnected)
    {
      Paint_DrawBitmap(112, 220, urlbmp_16x16, 16, 16, DMBLUE, MBLUE);
    }
    break;
  }
}

/**
 * Update displayed value
 * */
void updateGFX()
{
  switch (currentDisplay)
  {
  case oilpressure:
    oilPressure = random(48, 110);
    currentVal = oilPressure;
    break;

  case oiltemp:
    oilTemp = random(90, 110);
    currentVal = oilTemp;
    break;

  case watertemp:
    waterTemp = random(80, 110);
    currentVal = waterTemp;
    break;

  case rpm:
    currentVal = RPM;
    break;

  case manipressure:
    maniPress = random(48, 200);
    currentVal = maniPress;
    break;
  }

  xPos = 78;
  if (currentVal > 99)
  {
    xPos = 57;
  }
  if (currentVal > 999)
  {
    xPos = 36;
  }
  if ((prevVal > 999 && currentVal < 1000) || (prevVal > 99 && currentVal < 100) || (prevVal > 9 && currentVal < 10))
  {
    LCD_ClearWindow(0, 34, 240, 110, BLACK);
  }
  Paint_DrawNum(xPos, 34, currentVal, &FontPTMono70, BLACK, WHITE);
  Paint_DrawShortTime(88, 6, &currentTime, &FontBebasNeue20, GRAY, WHITE);
  prevVal = currentVal;
}

/**
 * Retrieve the current time
 * */
time_t getTeensy3Time()
{
  return Teensy3Clock.get();
}

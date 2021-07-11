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
#include <EEPROM.h>
#include <Bounce2.h>

// Constants
const double pi = 3.14159267;     /* Pi constant */
const int display_centre_x = 120; /* LCD centre X co-ordinate */
const int display_centre_y = 120; /* LCD centre Y co-ordinate */

bool OBDconnected = false; /* OBD connection flag */

elapsedMillis updateTimer;  /* Update timer */
uint32_t updateInt = 60000; /* Update timer interval */

int oilPressure, oilTemp, waterTemp, RPM, maniPress, ignAdv; /* Integer OBD values */
int prevVal, currentVal;                                     /* Integer value storage */

double afr1, afr2;            /* Floating point OBD values */
double fPrevVal, fCurrentVal; /* Floating point value storage */

bool doubleVal;

int xPos;        /* Starting position for display value */
int lcdBL = 255; /* LCD backlight level */

PAINT_TIME currentTime; /* Current time object */

// Clock drawing variables
int clock_x;  /* Clock X co-ordinate */
int clock_y;  /* Clock Y co-ordinate */
int clock_x1; /* Clock X1 co-ordinate */
int clock_y1; /* Clock Y1 co-ordinate */

enum Displays : byte
{
  oilpressure,
  oiltemp,
  watertemp,
  rpm,
  manipressure,
  ignadv,
  clock,
  o2bank1,
  o2bank2
};

Displays currentDisplay;

#define BUTTON_PIN 4
Bounce2::Button button = Bounce2::Button();

Displays &operator++(Displays &d); /* Override ++ operator to cycle through displays */

void setup()
{
  // Start OBD connection
  if (OBD2.begin())
  {
    OBDconnected = true;
  }

  // Read the last used display
  currentDisplay = (Displays)EEPROM.read(0);

  // Initialise IO
  Config_Init();

  button.attach(BUTTON_PIN, INPUT_PULLUP);
  button.interval(5);
  button.setPressedState(LOW);

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

  button.update();

  if (button.pressed())
  {
    currentDisplay = ++currentDisplay;
    drawGFX();
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
    LCD_ClearWindow(0, 0, 240, 32, DARKGREEN);
    LCD_ClearWindow(0, 118, 240, 150, DARKGREEN);
    LCD_ClearWindow(0, 150, 240, 240, LIGHTGREEN);
    Paint_DrawString_EN(89, 119, "PSI", &FontAvenirNext20, DARKGREEN, WHITE);
    Paint_DrawString_EN(78, 151, "OIL", &FontBebasNeue40, LIGHTGREEN, WHITE);
    Paint_DrawString_EN(64, 190, "PRESSURE", &FontBebasNeue20, LIGHTGREEN, WHITE);
    if (OBDconnected)
    {
      Paint_DrawBitmap(112, 6, urlbmp_16x16, 16, 16, LIGHTGREEN, DARKGREEN);
    }
    break;

  case oiltemp:
    LCD_Clear(BLACK);
    LCD_ClearWindow(0, 0, 240, 32, DARKGREEN);
    LCD_ClearWindow(0, 118, 240, 150, DARKGREEN);
    LCD_ClearWindow(0, 150, 240, 240, LIGHTGREEN);
    Paint_DrawString_EN(46, 119, "CELCIUS", &FontAvenirNext20, DARKGREEN, WHITE);
    Paint_DrawString_EN(78, 151, "OIL", &FontBebasNeue40, LIGHTGREEN, WHITE);
    Paint_DrawString_EN(91, 190, "TEMP", &FontBebasNeue20, LIGHTGREEN, WHITE);
    if (OBDconnected)
    {
      Paint_DrawBitmap(112, 6, urlbmp_16x16, 16, 16, LIGHTGREEN, DARKGREEN);
    }
    break;

  case watertemp:
    LCD_Clear(BLACK);
    LCD_ClearWindow(0, 0, 240, 32, DARKBLUE);
    LCD_ClearWindow(0, 118, 240, 150, DARKBLUE);
    LCD_ClearWindow(0, 150, 240, 240, LIGHTBLUE);
    Paint_DrawString_EN(46, 119, "CELCIUS", &FontAvenirNext20, DARKBLUE, WHITE);
    Paint_DrawString_EN(50, 151, "WATER", &FontBebasNeue40, LIGHTBLUE, WHITE);
    Paint_DrawString_EN(91, 190, "TEMP", &FontBebasNeue20, LIGHTBLUE, WHITE);
    if (OBDconnected)
    {
      Paint_DrawBitmap(112, 6, urlbmp_16x16, 16, 16, LIGHTBLUE, DARKBLUE);
    }
    break;

  case rpm:
    LCD_Clear(BLACK);
    LCD_ClearWindow(0, 0, 240, 32, DARKRED);
    LCD_ClearWindow(0, 118, 240, 150, DARKRED);
    LCD_ClearWindow(0, 150, 240, 240, RED);
    Paint_DrawString_EN(88, 119, "RPM", &FontAvenirNext20, DARKRED, WHITE);
    Paint_DrawString_EN(36, 151, "ENGINE", &FontBebasNeue40, RED, WHITE);
    Paint_DrawString_EN(85, 190, "SPEED", &FontBebasNeue20, RED, WHITE);
    if (OBDconnected)
    {
      Paint_DrawBitmap(112, 6, urlbmp_16x16, 16, 16, RED, DARKRED);
    }
    break;

  case manipressure:
    LCD_Clear(BLACK);
    LCD_ClearWindow(0, 0, 240, 32, DBLUE);
    LCD_ClearWindow(0, 118, 240, 150, DBLUE);
    LCD_ClearWindow(0, 150, 240, 240, BLUE);
    Paint_DrawString_EN(88, 119, "kPa", &FontAvenirNext20, DBLUE, WHITE);
    Paint_DrawString_EN(50, 151, "INLET", &FontBebasNeue40, BLUE, WHITE);
    Paint_DrawString_EN(64, 190, "PRESSURE", &FontBebasNeue20, BLUE, WHITE);
    if (OBDconnected)
    {
      Paint_DrawBitmap(112, 6, urlbmp_16x16, 16, 16, BLUE, DBLUE);
    }
    break;

  case ignadv:
    LCD_Clear(BLACK);
    LCD_ClearWindow(0, 0, 240, 32, DMBLUE);
    LCD_ClearWindow(0, 118, 240, 150, DMBLUE);
    LCD_ClearWindow(0, 150, 240, 240, MBLUE);
    Paint_DrawString_EN(78, 119, "BTDC", &FontAvenirNext20, DMBLUE, WHITE);
    Paint_DrawString_EN(50, 151, "SPARK", &FontBebasNeue40, MBLUE, WHITE);
    Paint_DrawString_EN(71, 190, "ADVANCE", &FontBebasNeue20, MBLUE, WHITE);
    if (OBDconnected)
    {
      Paint_DrawBitmap(112, 6, urlbmp_16x16, 16, 16, MBLUE, DMBLUE);
    }
    break;

  case clock:
    LCD_Clear(BLACK);
    updateInt = 0;
    break;

  case o2bank1:
    LCD_Clear(BLACK);
    LCD_ClearWindow(0, 0, 240, 32, DPURPLE);
    LCD_ClearWindow(0, 118, 240, 150, DPURPLE);
    LCD_ClearWindow(0, 150, 240, 240, PURPLE);
    Paint_DrawString_EN(88, 119, "AFR", &FontAvenirNext20, DPURPLE, WHITE);
    Paint_DrawString_EN(36, 151, "OXYGEN", &FontBebasNeue40, PURPLE, WHITE);
    Paint_DrawString_EN(64, 190, "SENSOR 1", &FontBebasNeue20, PURPLE, WHITE);
    if (OBDconnected)
    {
      Paint_DrawBitmap(112, 6, urlbmp_16x16, 16, 16, PURPLE, DPURPLE);
    }
    break;

  case o2bank2:
    LCD_Clear(BLACK);
    LCD_ClearWindow(0, 0, 240, 32, DPURPLE);
    LCD_ClearWindow(0, 118, 240, 150, DPURPLE);
    LCD_ClearWindow(0, 150, 240, 240, PURPLE);
    Paint_DrawString_EN(88, 119, "AFR", &FontAvenirNext20, DPURPLE, WHITE);
    Paint_DrawString_EN(36, 151, "OXYGEN", &FontBebasNeue40, PURPLE, WHITE);
    Paint_DrawString_EN(64, 190, "SENSOR 2", &FontBebasNeue20, PURPLE, WHITE);
    if (OBDconnected)
    {
      Paint_DrawBitmap(112, 6, urlbmp_16x16, 16, 16, PURPLE, DPURPLE);
    }
    break;
  }

  // Store current display
  EEPROM.write(0, (byte)currentDisplay);
}

/**
 * Update displayed value
 * */
void updateGFX()
{
  switch (currentDisplay)
  {
  case oilpressure:
    doubleVal = false;
    currentVal = oilPressure;
    break;

  case oiltemp:
    doubleVal = false;
    currentVal = oilTemp;
    break;

  case watertemp:
    doubleVal = false;
    currentVal = waterTemp;
    break;

  case rpm:
    doubleVal = false;
    currentVal = RPM;
    break;

  case manipressure:
    doubleVal = false;
    currentVal = maniPress;
    break;

  case ignadv:
    doubleVal = false;
    currentVal = ignAdv;
    break;

  case clock:
    if (updateTimer > updateInt)
    {
      LCD_Clear(BLACK);
      int hour = currentTime.Hour;
      int min = currentTime.Min;
      draw_hour(hour, min);
      draw_minute(min);
      draw_clock_face();
      updateInt = 60000;
      updateTimer = 0;
    }
    break;

  case o2bank1:
    doubleVal = true;
    fCurrentVal = afr1;
    break;

  case o2bank2:
    doubleVal = true;
    fCurrentVal = afr2;
    break;
  }

  if (currentDisplay != clock)
  {
    if (!doubleVal)
    {
      xPos = 99;
      if (currentVal > 9)
      {
        xPos = 78;
      }
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
      prevVal = currentVal;
    }
    else
    {
      UBYTE precision = 2;
      xPos = 57;
      if (fCurrentVal >= 10)
      {
        xPos = 36;
        precision = 3;
      }
      if (fCurrentVal >= 100)
      {
        xPos = 36;
        precision = 4;
      }
      if ((fPrevVal > 999 && fCurrentVal < 1000) || (fPrevVal > 99 && fCurrentVal < 100) || (fPrevVal > 9 && fCurrentVal < 10))
      {
        LCD_ClearWindow(0, 34, 240, 110, BLACK);
      }
      Paint_DrawFloatNum(xPos, 34, fCurrentVal, precision, &FontPTMono70, BLACK, WHITE);
      fPrevVal = fCurrentVal;
    }
  }
}

/**
 * Retrieve the current time
 * */
time_t getTeensy3Time()
{
  return Teensy3Clock.get();
}

/**
 * Draws the hour hand 
 * */
void draw_hour(int hour, int minute)
{

  clock_y = (90 * cos(pi - (2 * pi) / 12 * hour - (2 * PI) / 720 * minute)) + display_centre_y;
  clock_x = (90 * sin(pi - (2 * pi) / 12 * hour - (2 * PI) / 720 * minute)) + display_centre_x;
  clock_y1 = (90 * cos(pi - (2 * pi) / 12 * hour - (2 * PI) / 720 * minute)) + display_centre_y + 1;
  clock_x1 = (90 * sin(pi - (2 * pi) / 12 * hour - (2 * PI) / 720 * minute)) + display_centre_x + 1;

  Paint_DrawLine(display_centre_x, display_centre_y, clock_x, clock_y, WHITE, DOT_PIXEL_2X2, LINE_STYLE_SOLID);
  Paint_DrawLine(display_centre_x + 1, display_centre_y + 1, clock_x1, clock_y1, WHITE, DOT_PIXEL_2X2, LINE_STYLE_SOLID);
}

/**
 * Draws the minute hand
 * */
void draw_minute(int minute)
{
  clock_y = (110 * cos(pi - (2 * pi) / 60 * minute)) + display_centre_y;
  clock_x = (110 * sin(pi - (2 * pi) / 60 * minute)) + display_centre_x;
  Paint_DrawLine(display_centre_x, display_centre_y, clock_x, clock_y, WHITE, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
}

/**
 * Draws the clock face
 * */
void draw_clock_face()
{
  // draw hour pointers around the face of a clock
  for (int i = 0; i < 60; i++)
  {
    clock_y = (117 * cos(pi - (2 * pi) / 60 * i)) + display_centre_y;
    clock_x = (117 * sin(pi - (2 * pi) / 60 * i)) + display_centre_x;
    clock_y1 = (117 * cos(pi - (2 * pi) / 60 * i)) + display_centre_y;
    clock_x1 = (117 * sin(pi - (2 * pi) / 60 * i)) + display_centre_x;
    if (i % 5 == 0)
    {
      Paint_DrawLine(clock_x1, clock_y1, clock_x, clock_y, WHITE, DOT_PIXEL_2X2, LINE_STYLE_SOLID);
    }
    else
    {
      Paint_DrawLine(clock_x1, clock_y1, clock_x, clock_y, GRAY, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
    }
  }

  // Draw the center of the clock
  Paint_DrawCircle(display_centre_x, display_centre_y, 5, WHITE, DOT_PIXEL_1X1, DRAW_FILL_FULL);
}

/**
 * Cycle through displays
 * */
Displays &operator++(Displays &d)
{
  return d = (d == Displays::o2bank2) ? Displays::oilpressure : static_cast<Displays>(static_cast<byte>(d) + 1);
}
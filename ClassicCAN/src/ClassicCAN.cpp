/*
    OBDII CAN Interface Display
    Using source code supplied with the Waveshare 1.28" circular LCD.

    © 2021 Joe Mann. https://wiki.joeblogs.co

    Version history:
    2021-10-07        v1.0.1        Removed unused libraries. Updated CAN comms + display tweaks
    2021-07-04        v1.0.0        Initial release
*/

#include <elapsedMillis.h>
#include <TimeLib.h>
#include <font_Arial.h>
#include <SPI.h>
#include "LCD_Driver.h"
#include "GUI_Paint.h"
#include "image.h"
#include <EEPROM.h>
#include <Bounce2.h>
#include <FlexCAN_T4.h>

// Constants
const double pi = 3.14159267;     /* Pi constant */
const int display_centre_x = 120; /* LCD centre X co-ordinate */
const int display_centre_y = 120; /* LCD centre Y co-ordinate */

// CAN bus
FlexCAN_T4<CAN1, RX_SIZE_256, TX_SIZE_16> Can0;
const long loopDelay1 = 50; /* Make a request every 50ms */
unsigned long timeNow1 = 0;
bool OBDconnected = true; /* OBD connection flag */

// ECU CAN address
#define ECU_ADDR 0x700;

// OBD PIDs
const uint8_t COOLANT_TEMP = 0x05;
const uint8_t MANIFOLD_PRESSURE = 0x0B;
const uint8_t ENGINE_SPEED = 0x0C;
const uint8_t TIMING_ADV = 0x0E;
const uint8_t INTAKE_AIR_TEMP = 0x0F;
const uint8_t O2_BANK1 = 0x24;
const uint8_t O2_BANK2 = 0x25;
const uint8_t ECU_VOLTAGE = 0x42;
const uint8_t OIL_TEMP = 0x5C;

int oilPressure, oilTemp, waterTemp, RPM, maniPress, ignAdv; /* Integer OBD values */
int prevVal, currentVal;                                     /* Integer value storage */

float afr1, afr2;            /* Floating point OBD values */
float fPrevVal, fCurrentVal; /* Floating point value storage */

bool floatVal; /* Flag to denote floating point variable */

int xPos;        /* Starting position for display value */
int lcdBL = 255; /* LCD backlight level */

PAINT_TIME currentTime; /* Current time object */

// Clock drawing variables
int clock_x;     /* Clock X co-ordinate */
int clock_y;     /* Clock Y co-ordinate */
int clock_x1;    /* Clock X1 co-ordinate */
int clock_y1;    /* Clock Y1 co-ordinate */
int cmin, chour; /* Current minute, current hour */

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

// Functions
void drawGFX();
void updateGFX();
time_t getTeensy3Time();
void draw_hour(int hour, int minute);
void draw_minute(int minute);
void draw_clock_face();
void canSniff(const CAN_message_t &msg);
void canTx_OBD(uint8_t pid);

Displays &operator++(Displays &d); /* Override ++ operator to cycle through displays */

void setup()
{
  Serial.begin(9600);
  delay(100);

  // Start OBD connection
  Can0.begin();
  Can0.setBaudRate(500000);
  Can0.setMaxMB(16);
  Can0.enableFIFO();
  Can0.enableFIFOInterrupt();
  Can0.onReceive(canSniff);

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

  cmin = -1;

  // Initialise LCD
  LCD_Init();
  LCD_SetBacklight(lcdBL);
  Paint_NewImage(LCD_WIDTH, LCD_HEIGHT, 0, BLACK);

  // Draw graphics
  drawGFX();

  Serial.println(currentDisplay);
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

  // Update CAN data
  Can0.events();

  button.update();

  if (button.pressed())
  {
    currentDisplay = ++currentDisplay;
    drawGFX();
  };

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
    floatVal = false;
    currentVal = oilPressure;
    break;

  case oiltemp:
    if (millis() > timeNow1 + loopDelay1)
    {
      timeNow1 = millis();
      canTx_OBD(OIL_TEMP);
    }

    floatVal = false;
    currentVal = oilTemp;
    break;

  case watertemp:
    if (millis() > timeNow1 + loopDelay1)
    {
      timeNow1 = millis();
      canTx_OBD(COOLANT_TEMP);
    }
    floatVal = false;
    currentVal = waterTemp;
    break;

  case rpm:
    if (millis() > timeNow1 + loopDelay1)
    {
      timeNow1 = millis();
      canTx_OBD(ENGINE_SPEED);
    }
    floatVal = false;
    currentVal = RPM;
    break;

  case manipressure:
    if (millis() > timeNow1 + loopDelay1)
    {
      timeNow1 = millis();
      canTx_OBD(MANIFOLD_PRESSURE);
    }
    floatVal = false;
    currentVal = maniPress;
    break;

  case ignadv:
    if (millis() > timeNow1 + loopDelay1)
    {
      timeNow1 = millis();
      canTx_OBD(TIMING_ADV);
    }
    floatVal = false;
    currentVal = ignAdv;
    break;

  case clock:
    if (cmin != currentTime.Min)
    {
      LCD_Clear(BLACK);
      chour = currentTime.Hour;
      cmin = currentTime.Min;
      draw_hour(chour, cmin);
      draw_minute(cmin);
      draw_clock_face();
    }
    break;

  case o2bank1:
    canTx_OBD(O2_BANK1);
    floatVal = true;
    fCurrentVal = afr1;
    break;

  case o2bank2:
    canTx_OBD(O2_BANK2);
    floatVal = true;
    fCurrentVal = afr2;
    break;
  }

  if (currentDisplay != clock)
  {
    cmin = -1; // Force re-draw when returning to clock display
    if (!floatVal)
    {
      xPos = 99;
      if (currentVal > 9 || currentVal < 0)
      {
        xPos = 78;
      }
      if (currentVal > 99 || currentVal < -10)
      {
        xPos = 57;
      }
      if (currentVal > 999 || currentVal < -100)
      {
        xPos = 36;
      }
      if ((prevVal > 999 && abs(currentVal) < 1000) || (prevVal > 99 && abs(currentVal) < 100) || (prevVal > 9 && abs(currentVal) < 10))
      {
        LCD_ClearWindow(0, 34, 240, 110, BLACK);
      }
      String s = String(currentVal);
      const char *print = s.c_str();
      Paint_DrawString_EN(xPos, 34, print, &FontPTMono70, BLACK, WHITE);
      prevVal = abs(currentVal);
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

void canSniff(const CAN_message_t &msg)
{
  // Global callback to catch any CAN frame coming in
  switch (msg.buf[2])
  {
  case OIL_TEMP:
    oilTemp = msg.buf[3] - 40;
    break;
  case COOLANT_TEMP:
    waterTemp = msg.buf[3] - 40;
    break;
  case ENGINE_SPEED:
    RPM = ((256 * msg.buf[3]) + msg.buf[4]) * 4;
    break;
  case MANIFOLD_PRESSURE:
    maniPress = msg.buf[3];
    Serial.println(maniPress);
    break;
  case TIMING_ADV:
    ignAdv = (msg.buf[3] / 2) - 64;
    Serial.println(ignAdv);
    break;
  }
}

void canTx_OBD(uint8_t pid)
{ // Request function to ask for Engine Coolant Temp via OBD request
  CAN_message_t msgTx, msgRx;
  msgTx.buf[0] = 0x02;
  msgTx.buf[1] = 0x01;
  msgTx.buf[2] = pid;
  msgTx.buf[3] = 0;
  msgTx.buf[4] = 0;
  msgTx.buf[5] = 0;
  msgTx.buf[6] = 0;
  msgTx.buf[7] = 0;
  msgTx.len = 8;            // number of bytes in request
  msgTx.flags.extended = 0; // 11 bit header, not 29 bit
  msgTx.flags.remote = 0;
  msgTx.id = ECU_ADDR; // request header for OBD
  Can0.write(msgTx);
}
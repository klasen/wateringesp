/***************************************************
  This is our touchscreen painting example for the Adafruit ILI9341 Shield
  ----> http://www.adafruit.com/products/1651

  Check out the links above for our tutorials and wiring diagrams
  These displays use SPI to communicate, 4 or 5 pins are required to
  interface (RST is optional)
  Adafruit invests time and resources providing this open source code,
  please support Adafruit and open-source hardware by purchasing
  products from Adafruit!

  Written by Limor Fried/Ladyada for Adafruit Industries.
  MIT license, all text above must be included in any redistribution
 ****************************************************/

#include <SPI.h>
//#include <Wire.h>      // this is needed even tho we aren't using it
#include <Adafruit_ILI9341.h>
#include <XPT2046_Touchscreen.h>

//Include Basecamp in this sketch
#include <Basecamp.hpp>
#include <Configuration.hpp>

//Basecamp iot{Basecamp::SetupModeWifiEncryption::secured, Basecamp::ConfigurationUI::accessPoint};
Basecamp iot;

// This is calibration data for the raw touch data to the screen coordinates
#define TS_MINX 450
#define TS_MINY 300
#define TS_MAXX 3930
#define TS_MAXY 3900

// The XPT204 uses hardware SPI
#define TS_CS 22
XPT2046_Touchscreen ts = XPT2046_Touchscreen(TS_CS);

// The display also uses hardware SPI
#define TFT_CS 5
#define TFT_DC 16
#define TFT_RESET 17
#define TFT_LED 4
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RESET);

// Size of the color selection boxes and the paintbrush size
#define BOXSIZE 40
#define PENRADIUS 3
int oldcolor, currentcolor, brightness;

void setup(void)
{
  // while (!Serial);     // used for leonardo debugging

  Serial.begin(115200);
  Serial.println(F("Touch Paint!"));

  // Initialize Basecamp
  iot.begin();

  tft.begin();
  tft.setRotation(1);
  tft.fillScreen(ILI9341_BLACK);

  if (!ts.begin())
  {
    Serial.println("Couldn't start touchscreen controller");
    while (1)
      ;
  }
  Serial.println("Touchscreen started");
  ts.setRotation(1);

  // read diagnostics (optional but can help debug problems)
  uint8_t x = tft.readcommand8(ILI9341_RDMODE);
  Serial.print("Display Power Mode: 0x");
  Serial.println(x, HEX);
  x = tft.readcommand8(ILI9341_RDMADCTL);
  Serial.print("MADCTL Mode: 0x");
  Serial.println(x, HEX);
  x = tft.readcommand8(ILI9341_RDPIXFMT);
  Serial.print("Pixel Format: 0x");
  Serial.println(x, HEX);
  x = tft.readcommand8(ILI9341_RDIMGFMT);
  Serial.print("Image Format: 0x");
  Serial.println(x, HEX);
  x = tft.readcommand8(ILI9341_RDSELFDIAG);
  Serial.print("Self Diagnostic: 0x");
  Serial.println(x, HEX);

  Serial.print("height: ");
  Serial.print(tft.height());
  Serial.print(", width: ");
  Serial.println(tft.width());

  tft.fillScreen(ILI9341_BLACK);

  // make the color selection boxes
  tft.fillRect(0, 0, BOXSIZE, BOXSIZE, ILI9341_RED);
  tft.fillRect(BOXSIZE, 0, BOXSIZE, BOXSIZE, ILI9341_YELLOW);
  tft.fillRect(BOXSIZE * 2, 0, BOXSIZE, BOXSIZE, ILI9341_GREEN);
  tft.fillRect(BOXSIZE * 3, 0, BOXSIZE, BOXSIZE, ILI9341_CYAN);
  tft.fillRect(BOXSIZE * 4, 0, BOXSIZE, BOXSIZE, ILI9341_BLUE);
  tft.fillRect(BOXSIZE * 5, 0, BOXSIZE, BOXSIZE, ILI9341_MAGENTA);

  // select the current color 'red'
  tft.drawRect(0, 0, BOXSIZE, BOXSIZE, ILI9341_WHITE);
  currentcolor = ILI9341_RED;

  //pinMode(TFT_LED, OUTPUT);
  brightness = 100;
  dacWrite(DAC1, brightness);
}

void loop()
{

  // See if there's any  touch data for us
  if (ts.bufferEmpty())
  {
    return;
  }

  // You can also wait for a touch
  if (!ts.touched())
  {
    if (brightness < 70)
    {
      brightness++;
      dacWrite(DAC1, brightness);
      delay(20);
    }
    else
    {
      brightness = 0;
    }
    Serial.print("brightness = ");
    Serial.println(brightness);
    return;
  }

  // Retrieve a point
  TS_Point p = ts.getPoint();

  Serial.print("X = ");
  Serial.print(p.x);
  Serial.print("\tY = ");
  Serial.print(p.y);
  Serial.print("\tPressure = ");
  Serial.print(p.z);

  // Scale from ~0->4000 to tft.width using the calibration #'s
  p.x = map(TS_MAXX - p.x, TS_MINX, TS_MAXX, 0, tft.width()) + 40;

  p.y = map(p.y, TS_MINY, TS_MAXY, 0, tft.height());

  Serial.print("  (");
  Serial.print(p.x);
  Serial.print(", ");
  Serial.print(p.y);
  Serial.println(")");

  if (p.y < BOXSIZE)
  {
    oldcolor = currentcolor;

    if (p.x < BOXSIZE)
    {
      currentcolor = ILI9341_RED;
      tft.drawRect(0, 0, BOXSIZE, BOXSIZE, ILI9341_WHITE);
    }
    else if (p.x < BOXSIZE * 2)
    {
      currentcolor = ILI9341_YELLOW;
      tft.drawRect(BOXSIZE, 0, BOXSIZE, BOXSIZE, ILI9341_WHITE);
    }
    else if (p.x < BOXSIZE * 3)
    {
      currentcolor = ILI9341_GREEN;
      tft.drawRect(BOXSIZE * 2, 0, BOXSIZE, BOXSIZE, ILI9341_WHITE);
    }
    else if (p.x < BOXSIZE * 4)
    {
      currentcolor = ILI9341_CYAN;
      tft.drawRect(BOXSIZE * 3, 0, BOXSIZE, BOXSIZE, ILI9341_WHITE);
    }
    else if (p.x < BOXSIZE * 5)
    {
      currentcolor = ILI9341_BLUE;
      tft.drawRect(BOXSIZE * 4, 0, BOXSIZE, BOXSIZE, ILI9341_WHITE);
    }
    else if (p.x < BOXSIZE * 6)
    {
      currentcolor = ILI9341_MAGENTA;
      tft.drawRect(BOXSIZE * 5, 0, BOXSIZE, BOXSIZE, ILI9341_WHITE);
    }

    if (oldcolor != currentcolor)
    {
      if (oldcolor == ILI9341_RED)
        tft.fillRect(0, 0, BOXSIZE, BOXSIZE, ILI9341_RED);
      if (oldcolor == ILI9341_YELLOW)
        tft.fillRect(BOXSIZE, 0, BOXSIZE, BOXSIZE, ILI9341_YELLOW);
      if (oldcolor == ILI9341_GREEN)
        tft.fillRect(BOXSIZE * 2, 0, BOXSIZE, BOXSIZE, ILI9341_GREEN);
      if (oldcolor == ILI9341_CYAN)
        tft.fillRect(BOXSIZE * 3, 0, BOXSIZE, BOXSIZE, ILI9341_CYAN);
      if (oldcolor == ILI9341_BLUE)
        tft.fillRect(BOXSIZE * 4, 0, BOXSIZE, BOXSIZE, ILI9341_BLUE);
      if (oldcolor == ILI9341_MAGENTA)
        tft.fillRect(BOXSIZE * 5, 0, BOXSIZE, BOXSIZE, ILI9341_MAGENTA);
    }
  }
  if (((p.y - PENRADIUS) > BOXSIZE) && ((p.y + PENRADIUS) < tft.height()))
  {
    tft.fillCircle(p.x, p.y, PENRADIUS, currentcolor);
  }
}
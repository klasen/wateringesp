//Define DEBUG to get the Output from DEBUG_PRINTLN
#define DEBUG 1

#include <SPI.h>
#include <Adafruit_ILI9341.h>

#include <XPT2046_Touchscreen.h>

//Include Basecamp in this sketch
#include <Basecamp.hpp>
#include <Configuration.hpp>

//Basecamp iot{Basecamp::SetupModeWifiEncryption::secured, Basecamp::ConfigurationUI::accessPoint};
Basecamp iot(Basecamp::SetupModeWifiEncryption::none, Basecamp::ConfigurationUI::always);

// This is calibration data for the raw touch data to the screen coordinates
#define TS_MINX 450
#define TS_MINY 300
#define TS_MAXX 3930
#define TS_MAXY 3900

// The XPT204 uses hardware SPI
#define TS_CS 22
#define TS_IRQ 3
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

TS_Point p_old = TS_Point();

//This is used to control if the ESP should enter sleep mode or not
bool delaySleep = false;

//Variables for the mqtt packages and topics
uint16_t statusPacketIdSub = 0;
String delaySleepTopic;
String statusTopic;
String touchPointTopic;

// Reset the configuration to factory defaults (all empty)
void resetToFactoryDefaults()
{
  DEBUG_PRINTLN("Resetting to factory defaults");
  Configuration config(String{"/basecamp.json"});
  config.load();
  config.resetExcept({
      ConfigurationKey::accessPointSecret,
  });
  config.save();
}

//This function transfers the state of the sensor. That includes the door status, battery status and level
void transmitStatus()
{
  DEBUG_PRINTLN(__func__);

  char sensorC[18];
  //convert the sensor value to a string
  sprintf(sensorC, "%04i %04i %04i", p_old.x, p_old.y, p_old.z);
  //Send the sensor value to the MQTT broker
  iot.mqtt.publish(touchPointTopic.c_str(), 1, true, sensorC);

  DEBUG_PRINTLN("Data published");
}

//This function is called when the MQTT-Server is connected
void onMqttConnect(bool sessionPresent)
{
  DEBUG_PRINTLN(__func__);
  DEBUG_PRINTLN(iot.hostname);
  DEBUG_PRINTLN(iot.wifi.getIP().toString());
  

  //Subscribe to the delay topic
  iot.mqtt.subscribe(delaySleepTopic.c_str(), 0);
  //Trigger the transmission of the current state.
  transmitStatus();
}


//This topic is called if an MQTT message is received
void onMqttMessage(char *topic, char *payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total)
{
  DEBUG_PRINTLN(__func__);

  //Check if the payload eqals "true" and set delaySleep correspondigly
  //Since we only subscribed to one topic, we only have to compare the payload
  if (strcmp(payload, "true") == 0)
  {
    delaySleep = true;
  }
  else
  {
    delaySleep = false;
  }
}

void suspendESP(uint16_t packetId)
{
  DEBUG_PRINTLN(__func__);

  //Check if the published package is the one of the door sensor
  if (packetId == statusPacketIdSub)
  {

    if (delaySleep == true)
    {
      DEBUG_PRINTLN("Delaying Sleep");
      return;
    }
    DEBUG_PRINTLN("Entering deep sleep");
    //properly disconnect from the MQTT broker
    iot.mqtt.disconnect();
    //send the ESP into deep sleep
    esp_deep_sleep_start();
  }
}

void setup(void)
{
  // while (!Serial);     // used for leonardo debugging

  Serial.begin(115200);
  Serial.println(F("Touch Paint!"));

  // Initialize Basecamp
  iot.begin();
  // iot.wifi.begin("WLAN-Grundmann", "0043847185725197", "True", "WateringESP", "");
  // iot.mqtt.setServer("172.30.1.12", 1883);

  //Configure the MQTT topics
  delaySleepTopic = "cmd/" + iot.hostname + "/delaysleep";
  statusTopic = "stat/" + iot.hostname + "/status";
  touchPointTopic = "stat/" + iot.hostname + "/touchpoint";

  //Set up the Callbacks for the MQTT instance. Refer to the Async MQTT Client documentation
  // TODO: We should do this actually _before_ connecting the mqtt client...
  iot.mqtt.onConnect(onMqttConnect);
  // iot.mqtt.onPublish(suspendESP);
  iot.mqtt.onMessage(onMqttMessage);

  iot.mqtt.connect();

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

  p_old = ts.getPoint();


  
  Serial.print("wifi: ");
  Serial.print(iot.wifi.status());
  Serial.print(", wifi: ");
  // Serial.print(iot.wifi.;
  // Serial.print(", wifi: ");
  Serial.print(iot.wifi.getIP().toString());
  Serial.println();
}


void loop()
{

  // See if there's any  touch data for us
  if (ts.bufferEmpty())
  {
    return;
  }

  // Retrieve a point
  TS_Point p = ts.getPoint();
  if (p == p_old)
  {
    return;
  }
  p_old = p;

  Serial.print("X = ");
  Serial.print(p.x);
  Serial.print("\tY = ");
  Serial.print(p.y);
  Serial.print("\tPressure = ");
  Serial.print(p.z);

  transmitStatus();

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
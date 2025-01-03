/*#################################An example to connect thingcontro.io MQTT over TLS1.2###############################
*/
#include <Arduino.h>
#include <ArduinoOTA.h>
#include <EEPROM.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>
#include <WiFi.h>
#include <WiFiManager.h>
#include <WiFiClientSecure.h>
#include <Wire.h>
//  #include "Adafruit_MCP23008.h"    // i2c lib to control relay shield
// Modbus
//#include <ModbusMaster.h>
//#include "REG_CONFIG.h"
#include <HardwareSerial.h>
//#include <BME280I2C.h>
#include <sparkFunBME280.h>
#include <SparkFun_SGP30_Arduino_Library.h>
#include <TFT_eSPI.h>
#include <SPI.h>

#include <ESPUI.h>
#include <ESPmDNS.h>

//#include <TaskScheduler.h>
#include <http_ota.h>

//#include "Adafruit_SGP30.h"
#include <Adafruit_MLX90614.h>

#include "LogoAIS.h"
#include "lv1.h"
#include "lv2.h"
#include "lv3.h"
#include "lv4.h"
#include "lv5.h"
#include "lv6.h"
//  #include "NBIOT.h"
#include "wifilogo.h"
#include <time.h>
#include "Free_Fonts.h"

#include <ArduinoModbus.h>

WiFiClient wifiClient;
PubSubClient client(wifiClient);
WiFiManager wifiManager;
ModbusTCPClient modbusTCPClient(wifiClient);
IPAddress ip(192, 168, 0, 108);

TaskHandle_t Task1;

struct tm tmstruct;
struct tm timeinfo;

#define _TASK_SLEEP_ON_IDLE_RUN
#define _TASK_PRIORITY
#define _TASK_WDT_IDS
#define _TASK_TIMECRITICAL

//Scheduler runner;

boolean readPMSdata(Stream *s);
void composeJson();
void t1CallGetProbe();
void enterDetailsCallback(Control *sender, int type);
void sendAttribute();
void processAtt(char jsonAtt[]);
void reconnectMqtt();
void heartBeat();

void t1CallGetProbe();
void t2CallShowEnv();
void t3CallSendData();
void t4CallPrintPMS7003();
void t6OTA();
void t7showTime();
void enterConnectionCallback(Control *sender, int type);
void eepromWrite();
void holdingWrite(int address, uint16_t value);

int periodSendTelemetry = 60;  //the value is a number of seconds

/*
// Variables to keep track of the last execution time for each task
Task t1(60000, TASK_FOREVER, &t1CallGetProbe);  //adding task to the chain on creation
Task t2(60000, TASK_FOREVER, &t2CallShowEnv);
Task t3(300000, TASK_FOREVER, &t3CallSendData);
Task t4(300000, TASK_FOREVER, &t4CallPrintPMS7003);  //adding task to the chain on creation
Task t5(120000, TASK_FOREVER, &heartBeat);
Task t6(600000, TASK_FOREVER, &OTA_git_CALL);
Task t7(500, TASK_FOREVER, &t7showTime);

const unsigned long time2send = periodSendTelemetry * 1000;
Task t8(time2send, TASK_FOREVER, &composeJson);
*/

#define TFT_BLACK       0x0000      /*   0,   0,   0 */
#define TFT_NAVY        0x000F      /*   0,   0, 128 */
#define TFT_DARKGREEN   0x03E0      /*   0, 128,   0 */
#define TFT_DARKCYAN    0x03EF      /*   0, 128, 128 */
#define TFT_MAROON      0x7800      /* 128,   0,   0 */
#define TFT_PURPLE      0x780F      /* 128,   0, 128 */
#define TFT_OLIVE       0x7BE0      /* 128, 128,   0 */
#define TFT_LIGHTGREY   0xD69A      /* 211, 211, 211 */
#define TFT_DARKGREY    0x7BEF      /* 128, 128, 128 */
#define TFT_BLUE        0x001F      /*   0,   0, 255 */
#define TFT_GREEN       0x07E0      /*   0, 255,   0 */
#define TFT_CYAN        0x07FF      /*   0, 255, 255 */
#define TFT_RED         0xF800      /* 255,   0,   0 */
#define TFT_MAGENTA     0xF81F      /* 255,   0, 255 */
#define TFT_YELLOW      0xFEE0      /* 255, 255,   0 */
#define TFT_WHITE       0xFFFF      /* 255, 255, 255 */
#define TFT_ORANGE      0xFF83      /* 255, 180,   0 */
#define TFT_GREENYELLOW 0xB7E0      /* 180, 255,   0 */
#define TFT_PINK        0xFE19      /* 255, 192, 203 */
#define TFT_BROWN       0x9A60      /* 150,  75,   0 */
#define TFT_GOLD        0xFEA0      /* 255, 215,   0 */
#define TFT_SILVER      0xC618      /* 192, 192, 192 */
#define TFT_SKYBLUE     0x867D      /* 135, 206, 235 */
#define TFT_VIOLET      0x915C      /* 180,  46, 226 */

#define WDTPin 4
#define trigWDTPin 33
#define ledHeartPIN 0
#define swTFTPin 32

#define SERIAL1_RXPIN 25
#define SERIAL1_TXPIN 26

HardwareSerial hwSerial(2);
Adafruit_MLX90614 mlx = Adafruit_MLX90614();
SGP30 sgp;
BME280 bme;

uint32_t getAbsoluteHumidity(float temperature, float humidity) {
  // approximation formula from Sensirion SGP30 Driver Integration chapter 3.15
  const float absoluteHumidity = 216.7f * ((humidity / 100.0f) * 6.112f * exp((17.62f * temperature) / (243.12f + temperature)) / (273.15f + temperature)); // [g/m^3]
  const uint32_t absoluteHumidityScaled = static_cast<uint32_t>(1000.0f * absoluteHumidity); // [mg/m^3]
  return absoluteHumidityScaled;
}

#define WIFI_AP ""
#define WIFI_PASSWORD ""

String deviceToken = "";
char thingsboardServer[] = "tb.thingcontrol.io";
int PORT = 1883;

String host2 = "";
#define FORCE_USE_HOTSPOT 0

#define CF_OL24 &Orbitron_Light_24
#define CF_OL32 &Orbitron_Light_32

#define title1 "PM2.5" // Text that will be printed on screen in any font
#define title2 "PM1"
#define title3 "PM10"
#define title4 "CO2"
#define title5 "VOC"
#define title6 "Update"
#define title7 "ug/m3"
#define title8 "RH"
#define title9 "T"
#define FILLCOLOR1 0xFFFF

// Modbus
float temp(NAN), hum(NAN), pres(NAN);
int TempOffset = 0;
int HumOffset1 = 0;
int pm01Offset = 0;
int pm25Offset = 0;
int pm10Offset = 0;
int pn03Offset = 0;
int pn05Offset = 0;
int pn10Offset = 0;
int pn25Offset = 0;
int pn50Offset = 0;
int pn100Offset = 0;
int CO2Offset = 0;
int VOCOffset = 0;
String bmeStatus = "";
String mqttStatus = "";

#define TFT_BURGUNDY  0xF1EE

int xpos = 0;
int ypos = 0;

boolean ready2display = false;

int testNum = 0;
int wtd = 0;
int maxwtd = 10;

int tftMax = 160;
int _countFailed = 0;

bool connectWifi = false;

int TempAd, HumAd, PressAd;
int PM1Ad, PM25Ad, PM10Ad;
String ServAddress;

String json = "";

//ModbusMaster node;
///UI handles
uint16_t wifi_ssid_text, wifi_pass_text;
uint16_t nameLabel, idLabel, cuationlabel, firmwarelabel, mainSwitcher, mainSlider, mainText, settingZNumber, resultButton, mainTime, downloadButton, selectDownload, logStatus;
//  uint16_t styleButton, styleLabel, styleSwitcher, styleSlider, styleButton2, styleLabel2, styleSlider2;
uint16_t tempText, humText, humText2, saveConfigButton, interval ,emailText1;
uint16_t pm01Text, pm25Text, pm10Text, pn03Text, pn05Text, pn10Text, pn25Text, pn50Text, pn100Text, lineText;
uint16_t bmeLog, wifiLog, teleLog;
uint16_t tempAddr, humAddr, pressAddr, pm1Addr, pm25Addr, pm10Addr, serverIP;
uint16_t myIP;

uint16_t graph;
volatile bool updates = false;
String email1 = "";
String lineID = "";

// Basic toggle test for i/o expansion. flips pin #0 of a MCP23008 i2c
// pin expander up and down. Public domain

// Connect pin #1 of the expander to Analog 5 (i2c clock)
// Connect pin #2 of the expander to Analog 4 (i2c data)
// Connect pin #3, 4 and 5 of the expander to ground (address selection)
// Connect pin #6 and 18 of the expander to 5V (power and reset disable)
// Connect pin #9 of the expander to ground (common ground)

// Output #0 is on pin 10 so connect an LED or whatever from that to ground

//  Adafruit_MCP23008 mcp;

String imsi = "";
String NCCID = "";
boolean readPMS = false;
TFT_eSPI tft = TFT_eSPI();

TFT_eSprite stringPM25 = TFT_eSprite(&tft);
TFT_eSprite stringPM1 = TFT_eSprite(&tft);
TFT_eSprite stringPM10 = TFT_eSprite(&tft);
TFT_eSprite stringUpdate = TFT_eSprite(&tft);
TFT_eSprite stringCO2 = TFT_eSprite(&tft);
TFT_eSprite stringVOC = TFT_eSprite(&tft);

TFT_eSprite topNumber = TFT_eSprite(&tft);
TFT_eSprite ind = TFT_eSprite(&tft);
TFT_eSprite H = TFT_eSprite(&tft);
TFT_eSprite T = TFT_eSprite(&tft);

int status = WL_IDLE_STATUS;
String downlink = "";
char *bString;

struct pms7003data {
  uint16_t framelen;
  uint16_t pm10_standard, pm25_standard, pm100_standard;
  uint16_t pm01_env, pm25_env, pm100_env;
  uint16_t particles_03um, particles_05um, particles_10um, particles_25um, particles_50um, particles_100um;
  uint16_t unused;
  uint16_t checksum;
};

struct pms7003data data;


void splash() {
  int xpos =  0;
  int ypos = 40;
  tft.init();
  // Swap the colour byte order when rendering
  tft.setSwapBytes(true);
  tft.setRotation(1);  // landscape

  tft.fillScreen(TFT_BLACK);
  // Draw the icons
  tft.pushImage(tft.width() / 2 - logoWidth / 2, 39, logoWidth, logoHeight, Logo);
  tft.setTextColor(TFT_GREEN);
  tft.setTextDatum(TC_DATUM); // Centre text on x,y position

  tft.setFreeFont(FSB9);
  xpos = tft.width() / 2; // Half the screen width
  ypos = 150;
  String namePro = Project + "version" + FirmwareVer;
  tft.drawString(namePro, xpos, ypos + 25, GFXFF);  // Draw the text string in the selected GFX free font
  //  tft.drawString("", xpos, ypos + 20, GFXFF); // Draw the text string in the selected GFX free font
  //  AISnb.debug = true;
  //  AISnb.setupDevice(serverPort);
  //

  NCCID = deviceToken.c_str();
  NCCID.trim();
  String nccidStr = "";
  nccidStr.concat("Device ID:");
  nccidStr.concat(NCCID);
  //delay(4000);
  tft.setTextColor(TFT_WHITE);
  tft.setFreeFont(FSB9);
  tft.drawString(nccidStr, xpos, ypos + 45, GFXFF);
  delay(5000);

  tft.setTextFont(GLCD);

  tft.fillScreen(TFT_DARKCYAN);
  // Select the font
  ypos += tft.fontHeight(GFXFF);                      // Get the font height and move ypos down
  tft.setFreeFont(FSB9);
  //  tft.pushImage(tft.width() / 2 - (Splash2Width / 2) - 15, 3, Splash2Width, Splash2Height, Splash2);



  delay(1200);
  tft.setTextPadding(180);
  tft.setTextColor(TFT_GREEN);
  tft.setTextDatum(MC_DATUM);
  Serial.println(F("Start..."));
  for ( int i = 0; i < 170; i++)
  {
    tft.drawString("Waiting for WiFi...", xpos, 100, GFXFF);
    tft.drawString(".", 1 + 2 * i, 210, GFXFF);
    delay(10);
    //    Serial.println(i);
  }
  Serial.println("end");
}

void _initLCD() {
  pinMode(15, OUTPUT);
  digitalWrite(15, HIGH);

  tft.fillScreen(TFT_BLACK);
  // TFT
  splash();
  // MLX
  mlx.begin();
}

void errorTimeDisplay(int i) {
  tft.fillScreen(TFT_DARKCYAN);
  int xpos = tft.width() / 2; // Half the screen width
  int ypos = tft.height() / 2;
  tft.drawString("Connect Server failed " + String(i + 1) + " times", xpos, ypos, GFXFF);
}


char  char_to_byte(char c)
{
  if ((c >= '0') && (c <= '9'))
  {
    return (c - 0x30);
  }
  if ((c >= 'A') && (c <= 'F'))
  {
    return (c - 55);
  }
}
void drawUpdate(int num, int x, int y)
{
  stringUpdate.createSprite(50, 20);
  stringUpdate.fillScreen(TFT_BLACK);
  stringUpdate.setFreeFont(FSB9);
  stringUpdate.setTextColor(TFT_ORANGE);
  stringUpdate.setTextSize(1);
  stringUpdate.drawNumber(num, 0, 3);
  stringUpdate.drawString("%", 25, 3, GFXFF);
  stringUpdate.pushSprite(x, y);

}



void configModeCallback (WiFiManager *myWiFiManager) {
  Serial.println("Entered config mode");
  Serial.println(WiFi.softAPIP());
  //if you used auto generated SSID, print it
  Serial.println(myWiFiManager->getConfigPortalSSID());
}

void _initBME280()
{

  while (!Serial) {} // Wait

  delay(200);
  
  //Wire.begin(21, 22);
  Wire.begin();
  //Sensor supports I2C speeds up to 400kHz
  //Wire.setClock(400000);
  pinMode(32, OUTPUT); // on BME280
  digitalWrite(32, HIGH); // on BME280
  bme.setI2CAddress(0x76);

  while (!bme.beginI2C())
  {
    bmeStatus = "Could not find BME280 sensor!";
    Serial.println(bmeStatus);
    //  SerialBT.println("Could not find BME280 sensor!");
    ///ESPUI.updateLabel(bmeLog, String(bmeStatus));
    delay(1000);
  }

  bmeStatus = "Initialize BME sensor";
  //ESPUI.updateLabel(bmeLog, String(bmeStatus));
  /**
  // bme.chipID(); // Deprecated. See chipModel().
  switch (bme.chipModel())
  {
    case BME280::ChipModel_BME280:
    bmeStatus = "Found BME280 sensor! Success.";
      //      Serial.println(F("Found BME280 sensor! Success."));
      break;
    case BME280::ChipModel_BMP280:
    bmeStatus = "Found BMP280 sensor! No Humidity available.";
      //      Serial.println(F("Found BMP280 sensor! No Humidity available."));
      break;
    default:
    bmeStatus = "Found UNKNOWN sensor! Error!";
      Serial.println(F("Found UNKNOWN sensor! Error!"));

  }
   */
}

void printBME280Data()
{
  Serial.println("DebugprintStartBME280Data");
  //_initBME280();
  //  BME280::TempUnit tempUnit(BME280::TempUnit_Celsius);
  //  BME280::PresUnit presUnit(BME280::PresUnit_Pa);
  //  bme.read(pres, temp, hum, tempUnit, presUnit);

  temp = bme.readTempC();  //compensate

  hum = bme.readFloatHumidity();
  pres = bme.readFloatPressure();
  Serial.printf("Temp: %.2f, Hum: %.2f, Pres:: %.2f \n", temp, hum, pres);
  Serial.println("DebugprintEndBME280Data");
  holdingWrite(TempAd, temp);
  holdingWrite(HumAd, hum);
  holdingWrite(PressAd, pres);
}

void _initSGP30 () {
  //Initialize sensor
  if (sgp.begin() == false) {
    Serial.println("No SGP30 Detected. Check connections.");
    while (1);
  }
  //Initializes sensor for air quality readings
  //measureAirQuality should be called in one second increments after a call to initAirQuality
  sgp.initAirQuality();
  Serial.print("Found SGP30 serial #");
  sgp.getSerialID();
  //Get version number
  sgp.getFeatureSetVersion();
  Serial.print("SerialID: 0x");
  Serial.print((unsigned long)sgp.serialID, HEX);
  Serial.print("\tFeature Set Version: 0x");
  Serial.println(sgp.featureSetVersion, HEX);


}

void getDataSGP30 () {
  //First fifteen readings will be
  //CO2: 400 ppm  TVOC: 0 ppb
  delay(1000); //Wait 1 second
  //measure CO2 and TVOC levels
  sgp.measureAirQuality();
  Serial.print("CO2: ");
  Serial.print(sgp.CO2);
  Serial.print(" ppm\tTVOC: ");
  Serial.print(sgp.TVOC);
  Serial.println(" ppb");
}

// This is the main function which builds our GUI
// This is the main function which builds our GUI
void setUpUI() {

  //Turn off verbose  ging
  ESPUI.setVerbosity(Verbosity::Quiet);

  //Make sliders continually report their position as they are being dragged.
  ESPUI.sliderContinuous = true;

  //This GUI is going to be a tabbed GUI, so we are adding most controls using ESPUI.addControl
  //which allows us to set a parent control. If we didn't need tabs we could use the simpler add
  //functions like:
  //    ESPUI.button()
  //    ESPUI.label()
  
  /*
     Tab: Basic Controls
     This tab contains all the basic ESPUI controls, and shows how to read and update them at runtime.
    -----------------------------------------------------------------------------------------------------------*/
  auto maintab = ESPUI.addControl(Tab, "", "Home");
  nameLabel = ESPUI.addControl(Label, "Device Name", String(Project), Emerald, maintab);
  idLabel = ESPUI.addControl(Label, "Device ID", String(deviceToken), Emerald, maintab);
  firmwarelabel = ESPUI.addControl(Label, "Firmware", String(FirmwareVer), Emerald, maintab);
  myIP = ESPUI.addControl(Label, "IP Address", String(WiFi.localIP().toString()), Peterriver, maintab);

  auto settingTab = ESPUI.addControl(Tab, "", "Setting");
  cuationlabel = ESPUI.addControl(Label, "Cuation", "Offset will be divided by 100 after saving.", Emerald, settingTab);
  ESPUI.addControl(Separator, "Offset Configuration", "", None, settingTab);
  tempText = ESPUI.addControl(Number, "Temperature", String(TempOffset), Emerald, settingTab, enterDetailsCallback);
  humText = ESPUI.addControl(Number, "Humidity", String(HumOffset1), Emerald, settingTab, enterDetailsCallback);
  pm01Text = ESPUI.addControl(Number, "PM1.0", String(pm01Offset), Emerald, settingTab, enterDetailsCallback);
  pm25Text = ESPUI.addControl(Number, "PM2.5", String(pm25Offset), Emerald, settingTab, enterDetailsCallback);
  pm10Text = ESPUI.addControl(Number, "PM10", String(pm10Offset), Emerald, settingTab, enterDetailsCallback);
  ESPUI.addControl(Separator, "Offset Number of Particles Configuration", "", None, settingTab);
  pn03Text = ESPUI.addControl(Number, "0.3 micrometer", String(pn03Offset), Emerald, settingTab, enterDetailsCallback);
  pn05Text = ESPUI.addControl(Number, "0.5 micrometer", String(pn05Offset), Emerald, settingTab, enterDetailsCallback);
  pn10Text = ESPUI.addControl(Number, "1.0 micrometer", String(pn10Offset), Emerald, settingTab, enterDetailsCallback);
  pn25Text = ESPUI.addControl(Number, "2.5 micrometer", String(pn25Offset), Emerald, settingTab, enterDetailsCallback);
  pn50Text = ESPUI.addControl(Number, "5.0 micrometer", String(pn50Offset), Emerald, settingTab, enterDetailsCallback);
  pn100Text = ESPUI.addControl(Number, "10 micrometer", String(pn100Offset), Emerald, settingTab, enterDetailsCallback);
  //  CO2Text = ESPUI.addControl(Number, "Carbon dioxide (CO2)", String(CO2Offset), Emerald, settingTab, enterDetailsCallback);
  //  VOCText = ESPUI.addControl(Number, "Volatile organic Compounds", String(VOCOffset), Emerald, settingTab, enterDetailsCallback);
  
  
  ESPUI.addControl(Separator, "Interval Configuration", "", None, settingTab);
  interval = ESPUI.addControl(Number, "Interval (second)", String(periodSendTelemetry), Emerald, settingTab, enterDetailsCallback);
  
  /*
  ESPUI.addControl(Separator, "", "", None, settingTab);
  ESPUI.addControl(Button, "Save", "SAVE", Peterriver, settingTab, enterDetailsCallback);
  */
  
  ESPUI.addControl(Separator, "Email Registry", "", None, settingTab);
  /**/
  emailText1 = ESPUI.addControl(Text, "Email User 1", "", Emerald, settingTab, enterDetailsCallback);
  lineText = ESPUI.addControl(Text, "Line ID", "", Emerald, settingTab, enterDetailsCallback);

  /*
  emailText2 = ESPUI.addControl(Text, "Email User 2", email.email2, Emerald, settingTab, enterDetailsCallback);
  emailText3 = ESPUI.addControl(Text, "Email User 3", email.email3, Emerald, settingTab, enterDetailsCallback);
  emailText4 = ESPUI.addControl(Text, "Email User 4", email.email4, Emerald, settingTab, enterDetailsCallback);
  emailText5 = ESPUI.addControl(Text, "Email User 5", email.email5, Emerald, settingTab, enterDetailsCallback);
  /**/

  ESPUI.addControl(Separator, "", "", None, settingTab);
  //  ESPUI.addControl(Button, "Refresh", "Refresh", Peterriver, settingTab, enterDetailsCallback);
  ESPUI.addControl(Button, "Save", "SAVE", Peterriver, settingTab, enterDetailsCallback);

   auto network = ESPUI.addControl(Tab, "", "Connection");
  serverIP = ESPUI.addControl(Text, "Server IP Adress", String(ServAddress), Peterriver, network, enterConnectionCallback);
  tempAddr = ESPUI.addControl(Number, "Temperature Address", String(TempAd), Peterriver, network, enterConnectionCallback);
  humAddr = ESPUI.addControl(Number, "Humidity Address", String(HumAd), Peterriver, network, enterConnectionCallback);
  pressAddr = ESPUI.addControl(Number, "Pressure Address", String(PressAd), Peterriver, network, enterConnectionCallback);
  pm1Addr = ESPUI.addControl(Number, "PM1 Address", String(PM1Ad), Peterriver, network, enterConnectionCallback);
  pm25Addr = ESPUI.addControl(Number, "PM2.5 Address", String(PM25Ad), Peterriver, network, enterConnectionCallback);
  pm10Addr = ESPUI.addControl(Number, "PM10 Address", String(PM10Ad), Peterriver, network, enterConnectionCallback);
  ESPUI.addControl(Button, "Save", "SAVE", Dark, network, enterConnectionCallback);
  
  auto eventTab = ESPUI.addControl(Tab, "", "Event Log");
  ESPUI.addControl(Separator, "Error Log", "", None, eventTab);
  teleLog = ESPUI.addControl(Label, "Server Connection Status", String(mqttStatus), Alizarin, eventTab, enterDetailsCallback);
  bmeLog = ESPUI.addControl(Label, "Sensor Connection Status", String(bmeStatus), Alizarin, eventTab, enterDetailsCallback);
  host2 = "AIS-IoT:" + deviceToken;
  //Finally, start up the UI.
  //This should only be called once we are connected to WiFi.
  ESPUI.begin(host2.c_str());
  
  }

void enterConnectionCallback(Control *sender, int type) {
  Serial.println(sender->value);
  ESPUI.updateControl(sender);
  if(type == B_UP) {
    Serial.println("Saving Offset to EPROM...");
    Control* address_ = ESPUI.getControl(serverIP);
    Control* tempa_ = ESPUI.getControl(tempAddr);
    Control* huma_ = ESPUI.getControl(humAddr);
    Control* presa_ = ESPUI.getControl(pressAddr);
    Control* pm1a_ = ESPUI.getControl(pm1Addr);
    Control* pm25a_ = ESPUI.getControl(pm25Addr);
    Control* pm10a_ = ESPUI.getControl(pm10Addr);
    TempAd = tempa_->value.toInt();
    HumAd = huma_->value.toInt();
    PressAd = presa_->value.toInt();
    PM1Ad = pm1a_->value.toInt();
    PM25Ad = pm25a_->value.toInt();
    PM10Ad = pm10a_->value.toInt();
    ServAddress = address_->value.c_str();
    ip.fromString(ServAddress);
    eepromWrite();
  }
}

void enterDetailsCallback(Control *sender, int type) {
  Serial.println(sender->value);
  ESPUI.updateControl(sender);

  if(type == B_UP) {
      Serial.println("Saving Offset to EPROM...");

    // Fetch controls
    Control* TempOffset_ = ESPUI.getControl(tempText);
    Control* HumOffset1_ = ESPUI.getControl(humText);
    Control* pm01Offset_ = ESPUI.getControl(pm01Text);
    Control* pm25Offset_ = ESPUI.getControl(pm25Text);
    Control* pm10Offset_ = ESPUI.getControl(pm10Text);
    Control* pn03Offset_ = ESPUI.getControl(pn03Text);
    Control* pn05Offset_ = ESPUI.getControl(pn05Text);
    Control* pn10Offset_ = ESPUI.getControl(pn10Text);
    Control* pn25Offset_ = ESPUI.getControl(pn25Text);
    Control* pn50Offset_ = ESPUI.getControl(pn50Text);
    Control* pn100Offset_ = ESPUI.getControl(pn100Text);
    //  Control* CO2Offset_ = ESPUI.getControl(CO2Text);
    //  Control* VOCOffset_ = ESPUI.getControl(VOCText);
    Control* periodSendTelemetry_ = ESPUI.getControl(interval);
    Control* email1_ = ESPUI.getControl(emailText1);
    Control* lineID_ = ESPUI.getControl(lineText);
    
    // Store control values
    TempOffset = TempOffset_->value.toInt();
    HumOffset1 = HumOffset1_->value.toInt();
    pm01Offset = pm01Offset_->value.toInt();
    pm25Offset = pm25Offset_->value.toInt();
    pm10Offset = pm10Offset_->value.toInt();
    pn03Offset = pn03Offset_->value.toInt();
    pn05Offset = pn05Offset_->value.toInt();
    pn10Offset = pn10Offset_->value.toInt();
    pn25Offset = pn25Offset_->value.toInt();
    pn50Offset = pn50Offset_->value.toInt();
    pn100Offset = pn100Offset_->value.toInt();
    //  CO2Offset = CO2Offset_->value.toInt();
    //  VOCOffset = VOCOffset_->value.toInt();
    periodSendTelemetry = periodSendTelemetry_->value.toInt();
    email1 = email1_->value;
    lineID = lineID_->value;
    eepromWrite();
    sendAttribute(); // Assuming this function is required to send attributes
  }
}

void eepromWrite()
{
    char data1[40];
    char data2[40];
    char data3[40];
    email1.toCharArray(data1, 40);  // Convert String to char array
    lineID.toCharArray(data2, 40);
    ServAddress.toCharArray(data3, 20);
    //deviceToken.toCharArray(data3, 40);
    
    // Print to Serial
    Serial.println("put TempOffset: " + String(TempOffset));
    Serial.println("put HumOffset1: " + String(HumOffset1));
    Serial.println("put periodSendTelemetry: " + String(periodSendTelemetry));
    Serial.println("put email1: " + String(email1));
  // Write to EEPROM
    EEPROM.begin(200); // Ensure enough size for data
    int addr = 0;
  
    EEPROM.put(addr, TempOffset);
    addr += sizeof(TempOffset);
    EEPROM.put(addr, HumOffset1);
    addr += sizeof(HumOffset1);
    EEPROM.put(addr, pm01Offset);
    addr += sizeof(pm01Offset);
    EEPROM.put(addr, pm25Offset);
    addr += sizeof(pm25Offset);
    EEPROM.put(addr, pm10Offset);
    addr += sizeof(pm10Offset);
    EEPROM.put(addr, pn03Offset);
    addr += sizeof(pn03Offset);
    EEPROM.put(addr, pn05Offset);
    addr += sizeof(pn05Offset);
    EEPROM.put(addr, pn10Offset);
    addr += sizeof(pn10Offset);
    EEPROM.put(addr, pn25Offset);
    addr += sizeof(pn25Offset);
    EEPROM.put(addr, pn50Offset);
    addr += sizeof(pn50Offset);
    EEPROM.put(addr, pn100Offset);
    addr += sizeof(pn100Offset);
    /*
    EEPROM.put(addr, CO2Offset);
    addr += sizeof(CO2Offset);
    EEPROM.put(addr, VOCOffset);
    addr += sizeof(VOCOffset);
    */
    EEPROM.put(addr, periodSendTelemetry);
    addr += sizeof(periodSendTelemetry);

    EEPROM.put(addr, TempAd);
    addr += sizeof(TempAd);

    EEPROM.put(addr, HumAd);
    addr += sizeof(HumAd);

    EEPROM.put(addr, PressAd);
    addr += sizeof(PressAd);

    EEPROM.put(addr, PM1Ad);
    addr += sizeof(PM1Ad);

    EEPROM.put(addr, PM25Ad);
    addr += sizeof(PM25Ad);

    EEPROM.put(addr, PM10Ad);
    addr += sizeof(PM10Ad);

    for (int len = 0; len < ServAddress.length(); len++) {
      EEPROM.write(addr + len, data3[len]);  // Write each character
    }
    EEPROM.write(addr + ServAddress.length(), '\0');

    //addr = 70;
    for (int len = 0; len < email1.length(); len++) {
      EEPROM.write(addr + len, data1[len]);  // Write each character
    }
    EEPROM.write(addr + email1.length(), '\0');  // Add null terminator at the end
    
    //addr = 110;
    for (int len = 0; len < lineID.length(); len++) {
      EEPROM.write(addr + len, data2[len]);  // Write each character
    }
    EEPROM.write(addr + lineID.length(), '\0');  // Add null terminator at the end
    /*
    addr = 140;
    for (int len = 0; len < deviceToken.length(); len++) {
      EEPROM.write(addr + len, data3[len]);  // Write each character
    }
    EEPROM.write(addr + deviceToken.length(), '\0');  // Add null terminator at the end
    */
    EEPROM.commit();
    //  addr += (email1.length() + 1); For the future project
    //Dubug Address
    /*
    for (int i = 0; i < 50; i++) { // Reading the first 10 bytes
    Serial.print("Address ");
    Serial.print(i);
    Serial.print(": ");
    Serial.println(EEPROM.read(i)); // Likely will print 255
  }
  */
    EEPROM.end();
}

void readEEPROM() {
  Serial.println("Reading credentials from EEPROM...");
  EEPROM.begin(200); // Ensure enough size for data
  
  //Dubug Address
    /*
    for (int i = 0; i < 50; i++) { // Reading the first 10 bytes
    Serial.print("Address ");
    Serial.print(i);
    Serial.print(": ");
    Serial.println(EEPROM.read(i)); // Likely will print 255
  }
  */
  
  int addr = 0;
    EEPROM.get(addr, TempOffset);
    addr += sizeof(TempOffset);
    EEPROM.get(addr, HumOffset1);
    addr += sizeof(HumOffset1);
    EEPROM.get(addr, pm01Offset);
    addr += sizeof(pm01Offset);
    EEPROM.get(addr, pm25Offset);
    addr += sizeof(pm25Offset);
    EEPROM.get(addr, pm10Offset);
    addr += sizeof(pm10Offset);
    EEPROM.get(addr, pn03Offset);
    addr += sizeof(pn03Offset);
    EEPROM.get(addr, pn05Offset);
    addr += sizeof(pn05Offset);
    EEPROM.get(addr, pn10Offset);
    addr += sizeof(pn10Offset);
    EEPROM.get(addr, pn25Offset);
    addr += sizeof(pn25Offset);
    EEPROM.get(addr, pn50Offset);
    addr += sizeof(pn50Offset);
    EEPROM.get(addr, pn100Offset);
    addr += sizeof(pn100Offset);
    /*
    EEPROM.get(addr, CO2Offset);
    addr += sizeof(CO2Offset);
    EEPROM.get(addr, VOCOffset);
    addr += sizeof(VOCOffset);
    */
    EEPROM.get(addr, periodSendTelemetry);
    addr += sizeof(periodSendTelemetry);

    EEPROM.get(addr, TempAd);
    addr += sizeof(TempAd);

    EEPROM.get(addr, HumAd);
    addr += sizeof(HumAd);

    EEPROM.get(addr, PressAd);
    addr += sizeof(PressAd);

    EEPROM.get(addr, PM1Ad);
    addr += sizeof(PM1Ad);

    EEPROM.get(addr, PM25Ad);
    addr += sizeof(PM25Ad);

    EEPROM.get(addr, PM10Ad);
    addr += sizeof(PM10Ad);

    for (int len = 0; len < 20; len++){
    char data1 = EEPROM.read(addr + len);
    if (data1 == '\0' || data1 == 255)
      break;
    ServAddress += data1;
    }
    addr += sizeof(ServAddress);

    //addr = 70;
    for (int len = 0; len < 50; len++){
      char data1 = EEPROM.read(addr + len);
      if(data1 == '\0' || data1 == 255) break;
      email1 += data1;
    }
    addr += sizeof(email1);
    //addr = 110;
    for (int len = 0; len < 50; len++){
      char data2 = EEPROM.read(addr + len);
      if(data2 == '\0' || data2 == 255) break;
      lineID += data2;
    }
    /*
    addr = 140;
    for (int len = 0; len < 50; len++){
      char data3 = EEPROM.read(addr + len);
      if(data3 == '\0' || data3 == 255) break;
      deviceToken += data3;
    }
    */
    EEPROM.end();
    ip.fromString(ServAddress);
  // Print to Serial
  Serial.println("get TempOffset: " + String(TempOffset));
  Serial.println("get HumOffset1: " + String(HumOffset1));
  Serial.println("get periodSendTelemetry: " + String(periodSendTelemetry));
  Serial.println("get outputEmail1: " + String(email1));
  Serial.println("Server IP: " + String(ServAddress));

  //pm01Offset = 0;
  //pm25Offset = 0;
  //pm10Offset = 0;
  //pn03Offset = 0;
  //pn05Offset = 0;
  //pn10Offset = 0;
  //pn25Offset = 0;
  //pn50Offset = 0;
  //pn100Offset = 0;

  ESPUI.updateNumber(tempText, TempOffset);
  ESPUI.updateNumber(humText, HumOffset1);
  ESPUI.updateNumber(interval, periodSendTelemetry);
  ESPUI.updateNumber(pm01Text, pm01Offset);
  ESPUI.updateNumber(pm25Text, pm25Offset);
  ESPUI.updateNumber(pm10Text, pm10Offset);
  ESPUI.updateNumber(pn03Text, pn03Offset);
  ESPUI.updateNumber(pn05Text, pn05Offset);
  ESPUI.updateNumber(pn10Text, pn10Offset);
  ESPUI.updateNumber(pn25Text, pn25Offset);
  ESPUI.updateNumber(pn50Text, pn50Offset);
  ESPUI.updateNumber(pn100Text, pn100Offset);
  //  ESPUI.updateNumber(CO2Text, CO2Offset);
  //  ESPUI.updateNumber(VOCText, VOCOffset);
  ESPUI.updateText(emailText1, String(email1));
  ESPUI.updateText(lineText, String(lineID));
}
void processAtt(char jsonAtt[])
{
  char *aString = jsonAtt;
  Serial.println("OK");
  Serial.print(F("+:topic v1/devices/me/attributes , "));
  Serial.println(aString);
  client.publish( "v1/devices/me/attributes", aString);
}


void unrecognized(const char *command)
{
  Serial.println("ERROR");
}

void reconnectMqtt()
{
  if ( client.connect("Thingcontrol_AT", deviceToken.c_str(), NULL) )
  {
    mqttStatus = "Succeed to Connect Server!";
    ESPUI.updateLabel(teleLog, String(mqttStatus));
    Serial.println( F("Connect MQTT Success."));
    client.subscribe("v1/devices/me/rpc/request/+");
  }else{
    mqttStatus = "Failed to Connect Server!";
    ESPUI.updateLabel(teleLog, String(mqttStatus));
    ESP.restart();
  }
}

void sendAttribute(){
  String json = "";
  json.concat("{\"email1\":\"");
  json.concat(String(email1));
  //  json.concat("\",\"email2\":\"");
  //  json.concat(String(email.email2));
  json.concat("\"}");
  Serial.println(json);
  // Length (with one extra character for the null terminator)
  int str_len = json.length() + 1;
  // Prepare the character array (the buffer)
  char char_array[str_len];
  // Copy it over
  json.toCharArray(char_array, str_len);
  processAtt(char_array);
}



void t1CallGetProbe() {

  pinMode(33, OUTPUT); // turn on PMS7003
  digitalWrite(33, HIGH); // turn on PMS7003

  boolean pmsReady = readPMSdata(&hwSerial);
  Serial.print("pmsReady:");
  Serial.println(pmsReady);
  if ( pmsReady ) {
    ready2display = true;
    wtd = 0;
  } else {
    ready2display = false;
    _countFailed--;
    if (_countFailed >= 50000) {
      Serial.print("t1CallGetProbe.restart():" ); Serial.println(_countFailed);

      ESP.restart();
    } else {

      Serial.print("t1CallGetProbe.failed:" ); Serial.println(_countFailed);
      //modbus.begin(9600, SERIAL_8N1, SERIAL1_RXPIN, SERIAL1_TXPIN);
      hwSerial.begin(9600, SERIAL_8N1, SERIAL1_RXPIN, SERIAL1_TXPIN);

      _countFailed++;
    }

  }


  _initBME280();
  printBME280Data();
  _initSGP30();
  getDataSGP30();
}

void drawPM2_5(int num, int x, int y)
{
  // Create a sprite 80 pixels wide, 50 high (8kbytes of RAM needed)
  stringPM25.createSprite(175, 75);
  //  stringPM25.fillSprite(TFT_YELLOW);
  stringPM25.setTextSize(3);           // Font size scaling is x1
  stringPM25.setFreeFont(CF_OL24);  // Select free font

  stringPM25.setTextColor(TFT_WHITE);


  stringPM25.setTextSize(3);

  int mid = (tftMax / 2) - 1;

  stringPM25.setTextColor(TFT_WHITE);  // White text, no background colour
  // Set text coordinate datum to middle centre
  stringPM25.setTextDatum(MC_DATUM);
  // Draw the number in middle of 80 x 50 sprite
  stringPM25.drawNumber(num, mid, 25);
  // Push sprite to TFT screen CGRAM at coordinate x,y (top left corner)
  stringPM25.pushSprite(x, y);
  // Delete sprite to free up the RAM
  stringPM25.deleteSprite();
}

void drawT(float num, int x, int y)
{
  T.createSprite(40, 20);
  T.fillSprite(TFT_BLACK);
  T.setFreeFont(FSB9);
  T.setTextColor(TFT_WHITE);
  T.setTextSize(1);
  String myString = "";     // empty string
  myString.concat(num);
  T.drawString(myString, 0, 3);
  T.pushSprite(x, y);
  //  T.deleteSprite();
}

void drawH(float num, int x, int y)
{
  H.createSprite(40, 20);
  //  stringPM1.fillSprite(TFT_GREEN);
  H.setFreeFont(FSB9);
  H.setTextColor(TFT_WHITE);
  H.setTextSize(1);
  String myString = "";     // empty string
  myString.concat(num);
  H.drawString(myString, 0, 3);
  H.pushSprite(x, y);
  H.deleteSprite();
}


void drawPM1(int num, int x, int y)
{
  stringPM1.createSprite(50, 20);
  stringPM1.fillSprite(TFT_BLACK);
  stringPM1.setFreeFont(FSB9);
  stringPM1.setTextColor(TFT_WHITE);
  stringPM1.setTextSize(1);
  stringPM1.drawNumber(num, 0, 3);
  stringPM1.pushSprite(x, y);
  //  stringPM1.deleteSprite();
}
//


void drawPM10(int num, int x, int y)
{
  stringPM10.createSprite(50, 20);
  //  stringPM1.fillSprite(TFT_GREEN);
  stringPM10.setFreeFont(FSB9);
  stringPM10.setTextColor(TFT_WHITE);
  stringPM10.setTextSize(1);
  stringPM10.drawNumber(num, 0, 3);
  stringPM10.pushSprite(x, y);
  stringPM10.deleteSprite();
}

void t3CallSendData() {
  Serial.println("Startt3CallSendData()");
  digitalWrite(12, HIGH);
  delay(2000);
  digitalWrite(12, LOW);
  delay(200);
  tft.setTextColor(0xFFFF);
  int mapX = 315;
  int mapY = 30;
  Serial.println(WL_CONNECTED); Serial.print("(WiFi.status():"); Serial.println(WiFi.status());
  if (connectWifi == false) {
    Serial.println("connectwifi boolean: " + String(connectWifi));
    // if (AISnb.pingIP(serverIP).status == false) {
    //  ESP.restart();
    // }
    int rssi = map(WiFi.RSSI(), -110, -50, 25, 100);
    if (rssi > 100) rssi = 100;
    if (rssi < 0) rssi = 0;
    tft.fillRect(275, 5, 45, 35, 0x0000);
    tft.drawString(String(rssi)  + "%", mapX, mapY, GFXFF);
    tft.pushImage(240, 0, wifilogoWidth, wifilogoHeight, wifilogo);
  } else if (WiFi.status() == WL_CONNECTED) {
    int rssi = map(WiFi.RSSI(), -90, -50, 25, 100);
    if (rssi > 100) rssi = 100;
    if (rssi < 0) rssi = 0;
    tft.fillRect(275, 5, 45, 35, 0x0000);
    tft.drawString(String(rssi) + "%", mapX, mapY, GFXFF);
    tft.fillCircle(256, 16, 16, 0x9E4A);
    tft.setTextColor(0x0000);
    tft.setFreeFont(FSSB9);
    tft.drawString("W", 265, 27);
    //client.setInsecure();
    Serial.print(" deviceToken.c_str()"); Serial.println(deviceToken.c_str());
  }
  Serial.println("Endt3CallSendData()");


}
void composeJson() {
  json = "";
  Serial.println("StartcomposeJson");
  ////SerialBT.println(deviceToken);
  json.concat("{\"Tn\":\"");
  json.concat(deviceToken);
  json.concat("\",\"temp\":");
  //json.concat(String(25));
  json.concat(temp + (TempOffset / 100));
  json.concat(",\"hum\":");
  //json.concat(String(25));
  json.concat(hum + (HumOffset1 / 100));
  json.concat(",\"pres\":");
  //json.concat(String(25));
  json.concat(pres);
  json.concat(",\"pm1\":");
  json.concat(data.pm01_env + (pm01Offset / 100));
  json.concat(",\"pm2.5\":");
  json.concat(data.pm25_env + (pm25Offset / 100));
  json.concat(",\"pm10\":");
  json.concat(data.pm100_env + (pm10Offset / 100));

  json.concat(",\"pn03\":");
  json.concat(data.particles_03um + (pn03Offset / 100));
  json.concat(",\"pn05\":");
  json.concat(data.particles_05um + (pn05Offset / 100));
  json.concat(",\"pn10\":");
  json.concat(data.particles_10um + (pn10Offset / 100));
  json.concat(",\"pn25\":");
  json.concat(data.particles_25um + (pn25Offset / 100));
  json.concat(",\"pn50\":");
  json.concat(data.particles_50um + (pn50Offset / 100));
  json.concat(",\"pn100\":");
  json.concat(data.particles_100um + (pn100Offset / 100));
  
  json.concat(",\"project\":");
  json.concat(Project);
  json.concat(",\"v\":");
  json.concat(FirmwareVer);
  json.concat("}");
  Serial.println(json);
  //SerialBT.println(json);
  ////SerialBT.println(json);
  if (data.pm25_env > 1000){
    ESP.restart();
  }
  // Length (with one extra character for the null terminator)
  int str_len = json.length() + 1;
  // Prepare the character array (the buffer)
  char char_array[str_len];
  // Copy it over
  json.toCharArray(char_array, str_len);
  client.publish("v1/devices/me/telemetry", char_array);
}

void t4CallPrintPMS7003() {
Serial.println("Start Loop t4CallPrintPMS7003");
  // reading data was successful!
  //  Serial.println();
  //  Serial.println("---------------------------------------");
  //    Serial.println("Concentration Units (standard)");
  //    Serial.print("PM 1.0: "); Serial.print(data.pm10_standard);
  //    Serial.print("\t\tPM 2.5: "); Serial.print(data.pm25_standard);
  //    Serial.print("\t\tPM 10: "); Serial.println(data.pm100_standard);
  //    Serial.println("---------------------------------------");
  Serial.println("Concentration Units (environmental)");
  Serial.print("PM1.0:"); Serial.print(data.pm01_env);
  Serial.print("\tPM2.5:"); Serial.print(data.pm25_env);
  Serial.print("\tPM10:"); Serial.println(data.pm100_env);
  Serial.println("---------------------------------------");
  Serial.print("Particles > 0.3um / 0.1L air:"); Serial.println(data.particles_03um);
  Serial.print("Particles > 0.5um / 0.1L air:"); Serial.println(data.particles_05um);
  Serial.print("Particles > 1.0um / 0.1L air:"); Serial.println(data.particles_10um);
  Serial.print("Particles > 2.5um / 0.1L air:"); Serial.println(data.particles_25um);
  Serial.print("Particles > 5.0um / 0.1L air:"); Serial.println(data.particles_50um);
  Serial.print("Particles > 10.0 um / 0.1L air:"); Serial.println(data.particles_100um);
  Serial.println("---------------------------------------");

  holdingWrite(PM1Ad, data.pm01_env);
  holdingWrite(PM25Ad, data.pm25_env);
  holdingWrite(PM10Ad, data.pm100_env);

}

void heartBeat()
{

  
   //   Sink current to drain charge from watchdog circuit

  digitalWrite(WDTPin, LOW);
  delay(100);
  digitalWrite(WDTPin, HIGH);
 
 

  Serial.println("Heartbeat");
  // SerialBT.println("Heartbeat");
}

void t2CallShowEnv() {
  //  Serial.print(F("ready2display:"));
  //  Serial.println(ready2display);
  if (ready2display) {

    tft.setTextDatum(MC_DATUM);
    xpos = tft.width() / 2 ; // Half the screen width

    tft.setTextColor(TFT_WHITE);
    tft.setTextSize(1);
    tft.setFreeFont(CF_OL24);
    int mid = (tftMax / 2) - 72;
    tft.setTextPadding(100);
    tft.drawString(title7, xpos - 70, 125, GFXFF); // Print the test text in the custom font

    tft.setFreeFont(CF_OL32);
    tft.drawString(title1, xpos - 70, 155, GFXFF); // Print the test text in the custom font

    // ################################################################ for testing
    //        data.pm25_env = testNum;    //for testing
    //        testNum++;
    // ################################################################ end test


    drawPM2_5(data.pm25_env + (pm25Offset / 100), mid, 45);

    tft.setTextSize(1);
    tft.setFreeFont(CF_OL32);                 // Select the font

    tft.setTextDatum(BR_DATUM);
    tft.setTextColor(TFT_WHITE);
    tft.setTextSize(1);

    tft.setFreeFont(FSB9);   // Select Free Serif 9 point font, could use:

    drawPM1(data.pm01_env + (pm01Offset / 100), 6, 195);
    tft.drawString(title2, 40, 235, GFXFF); // Print the test text in the custom font

    drawPM10(data.pm100_env + (pm10Offset / 100), 55, 195);
    tft.drawString(title3, 100, 235, GFXFF); // Print the test text in the custom font

    
    tft.drawString(title8, 250, 215, GFXFF); // Print the test text in the custom font
    drawH(hum + (HumOffset1 / 100), 255, 195);
    tft.drawString("%", 312, 215, GFXFF);

    tft.drawString(title9, 250, 235, GFXFF); // Print the test text in the custom font
    drawT(temp + (TempOffset / 100), 255, 215);
    tft.drawString("C", 312, 235, GFXFF);

    //Clear Stage

    ind.createSprite(320, 10);
    ind.fillSprite(TFT_BLACK);

    if ((data.pm25_env + (pm25Offset / 100) >= 0) && (data.pm25_env + (pm25Offset / 100) <= 15.4)) {
      tft.setWindow(0, 25, 55, 55);
      tft.pushImage(tft.width() - lv1Width - 6, 45, lv1Width, lv1Height, lv1);
      ind.fillTriangle(0, 0, 5, 5, 10, 0, FILLCOLOR1);
    } else if ((data.pm25_env + (pm25Offset / 100) >= 15.5) && (data.pm25_env + (pm25Offset / 100) <= 40.4)  ) {
      tft.pushImage(tft.width() - lv2Width - 6, 45, lv2Width, lv2Height, lv2);
      ind.fillTriangle(55, 0, 60, 5, 65, 0, FILLCOLOR1);
    } else  if ((data.pm25_env + (pm25Offset / 100) >= 40.5) && (data.pm25_env + (pm25Offset / 100) <= 65.4)  ) {
      tft.pushImage(tft.width() - lv3Width - 6, 45, lv3Width, lv3Height, lv3);
      ind.fillTriangle(105, 0, 110, 5, 115, 0, FILLCOLOR1);
    } else  if ((data.pm25_env + (pm25Offset / 100) >= 65.5) && (data.pm25_env + (pm25Offset / 100) <= 150.4)  ) {
      tft.pushImage(tft.width() - lv4Width - 6, 45, lv4Width, lv4Height, lv4);
      ind.fillTriangle(155, 0, 160, 5, 165, 0, FILLCOLOR1);
    } else  if ((data.pm25_env + (pm25Offset / 100) >= 150.5) && (data.pm25_env + (pm25Offset / 100) <= 250.4)  ) {
      tft.pushImage(tft.width() - lv5Width - 6, 45, lv5Width, lv5Height, lv5);
      ind.fillTriangle(210, 0, 215, 5, 220, 0, FILLCOLOR1);
    } else {
      tft.pushImage(tft.width() - lv6Width - 6, 45, lv6Width, lv6Height, lv6);
      ind.fillTriangle(265, 0, 270, 5, 275, 0, FILLCOLOR1);
    }
    ind.pushSprite(29, 175);
    ind.deleteSprite();
  }
}

String a0(int n) {
  return (n < 10) ? "0" + String(n) : String(n);
}
String dateTimeStr = "";
  long timezone = 7;
  byte daysavetime = 0;

void t7showTime() {
  topNumber.createSprite(200, 40);
  //  stringPM1.fillSprite(TFT_GREEN);
  topNumber.setFreeFont(FS9);
  topNumber.setTextColor(TFT_WHITE);
  topNumber.setTextSize(1);           // Font size scaling is x1
  String yearStr = "";
  String monthStr = "";
  String dayStr = "";
  String hourStr = "";
  String minStr = "";
  tmstruct.tm_year = 0;
  configTime(3600 * timezone, daysavetime * 3600, "pool.ntp.org");
  getLocalTime(&tmstruct, 5000);
  yearStr = String(tmstruct.tm_year + 1900, DEC);
  monthStr = String(tmstruct.tm_mon + 1, DEC);
  dayStr = String(tmstruct.tm_mday, DEC);
  hourStr = String(a0(tmstruct.tm_hour));
  minStr = String(a0(tmstruct.tm_min));

  //  unsigned long NowTime = _epoch + ((millis() - time_s) / 1000) + (7 * 3600);
  String timeS = "";

  if (connectWifi == false) {
    timeS = dayStr + "/" + monthStr + "/" + yearStr + "  [" + hourStr + ":" + minStr + "]";

  } else {
    if (!getLocalTime(&timeinfo)) {
      Serial.println("Failed to obtain time");
      //      ESP.restart();
      return;
    }
  }

  topNumber.drawString(timeS, 5, 10, GFXFF);

  topNumber.pushSprite(5, 5);
  topNumber.deleteSprite();


}



boolean readPMSdata(Stream *s) {
  Serial.println("readPMSdata");
  if (! s->available()) {
    Serial.println("readPMSdata.false");
    //SerialBT.println("readPMSdata.false");

    return false;
  }

  // Read a byte at a time until we get to the special '0x42' start-byte
  if (s->peek() != 0x42) {
    s->read();
    return false;
  }

  // Now read all 32 bytes
  if (s->available() < 32) {
    return false;
  }

  uint8_t buffer[32];
  uint16_t sum = 0;
  s->readBytes(buffer, 32);

  // The data comes in endian'd, this solves it so it works on all platforms
  uint16_t buffer_u16[15];
  for (uint8_t i = 0; i < 15; i++) {
    buffer_u16[i] = buffer[2 + i * 2 + 1];
    buffer_u16[i] += (buffer[2 + i * 2] << 8);
  }

  memcpy((void *)&data, (void *)buffer_u16, 30);
  // get checksum ready
  for (uint8_t i = 0; i < 30; i++) {
    sum += buffer[i];
  }
  if (sum != data.checksum) {
    Serial.println("Checksum failure");
    return false;
  }
  // success!
  return true;
}

void getMac()
{
  byte mac[6];
  WiFi.macAddress(mac);
  Serial.println("OK");
  Serial.print("+deviceToken: ");
  Serial.println(WiFi.macAddress());
  for (int i = 0; i < 6; i++) {
    if (mac[i] < 0x10) {
      deviceToken += "0"; // Add leading zero if needed
    }
    deviceToken += String(mac[i], HEX); // Convert byte to hex
  }
  deviceToken.toUpperCase();
  ESPUI.updateLabel(idLabel, String(deviceToken));
}

void Task1code(void *pvParameters)
{

  for (;;)
  {
    //    Serial.print("Task1 running on core ");
    //    Serial.println(xPortGetCoreID());
    heartBeat();
    vTaskDelay((120000) / portTICK_PERIOD_MS);
  }
}

void _initModbus()
{
  while(!modbusTCPClient.begin(ip)) {
    Serial.println("Failed to connect Modbus TCP Server!");
    delay(1000);
  }
  Serial.println("Modbus TCP Client connected");
}

void reConnectionSever() {
  // Check for new client connections

  if (!modbusTCPClient.connected()) {
    Serial.println("Attempting to connect to Modbus TCP server");
    _initModbus();

  }
}

void holdingWrite(int address, uint16_t value)
{
  // Simulate a value for holding register 0
  modbusTCPClient.holdingRegisterWrite(address, value);
  Serial.printf("Register %d value: %d\n", address, value);
  //delay(1000); // Update register every second
}

void setup() {
  Serial.begin(115200);
 
  pinMode(WDTPin, OUTPUT);
  xTaskCreate(Task1code, "Task1", 10000, NULL, tskIDLE_PRIORITY, NULL);

  Wire.begin();
  
  Project = "AirmassTCP";
  FirmwareVer = "0.0";
  Serial.println(F("Starting... SHT20 TEMP/HUM_RS485 Monitor"));
  // communicate with Modbus slave ID 1 over Serial (port 2)
  getMac();
  
  
  Serial.println();
  Serial.println(F("***********************************"));
  //wifiManager.resetSettings();
  String host = "SmartEnv:" + deviceToken;
  wifiManager.setAPCallback(configModeCallback);
  if (!wifiManager.autoConnect(host.c_str())) {
    Serial.println("failed to connect and hit timeout");
    //reset and try again, or maybe put it to deep sleep
    // ESP.reset();
    
    delay(1000);
  }
  delay(1);
  

  configTime(3600 * timezone, daysavetime * 3600, "0.pool.ntp.org", "1.pool.ntp.org", "time.nist.gov");
 

  //  wifiClient.setFingerprint(fingerprint);
  client.setServer( thingsboardServer, PORT );
  //  client.setCallback(callback);
  reconnectMqtt();
  delay(1);
  
  Serial.print("Start..");
  tft.fillScreen(TFT_DARKCYAN);
  tft.drawString("Wait for WiFi Setting (Timeout 60 Sec)", tft.width() / 2, tft.height() / 2, GFXFF);
  delay(200);
  
  readEEPROM();
  host2 = "AIS-IoT:" + deviceToken;
  MDNS.begin(host2.c_str());
  WiFi.softAPConfig(IPAddress(192, 168, 1, 1), IPAddress(192, 168, 1, 1), IPAddress(255, 255, 255, 0));
  WiFi.softAP(host2.c_str());

  setUpUI(); //Start the GUI

  hwSerial.begin(9600, SERIAL_8N1, SERIAL1_RXPIN, SERIAL1_TXPIN);
  _initLCD();
  _initBME280();
  _initSGP30();
  delay(1);

  Serial.println("Start Modbus...");
  _initModbus();
  delayMicroseconds(1000);
  
  
  for (int i = 0; i < 1000; i++);
  tft.fillScreen(TFT_BLACK);            // Clear screen

  tft.fillRect(5, 185, tft.width() - 15, 5, TFT_GREEN); // Print the test text in the custom font
  tft.fillRect(63, 185, tft.width() - 15, 5, TFT_YELLOW); // Print the test text in the custom font
  tft.fillRect(113, 185, tft.width() - 15, 5, TFT_ORANGE); // Print the test text in the custom font
  tft.fillRect(166, 185, tft.width() - 15, 5, TFT_RED); // Print the test text in the custom font
  tft.fillRect(219, 185, tft.width() - 15, 5, TFT_PURPLE); // Print the test text in the custom font
  tft.fillRect(272, 185, tft.width() - 15, 5, TFT_BURGUNDY); // Print the test text in the custom font
  t1CallGetProbe();
  t2CallShowEnv();
  t3CallSendData();
  t4CallPrintPMS7003();
  t7showTime();

  

}

// Variables to keep track of the last execution time for each task


void loop() {
  //runner.execute();
  
  const unsigned long currentMillis = millis();
  const unsigned long time2send = periodSendTelemetry * 1000;
  if (currentMillis % time2send == 0){
    if (!client.connected())
    {
      reconnectMqtt();
    }
    composeJson();
    delayMicroseconds(200000);
    reConnectionSever();
  }
  if (currentMillis % 10000 == 0){
    t3CallSendData();
    t4CallPrintPMS7003();
    t1CallGetProbe();
    t2CallShowEnv();
  }
  if (currentMillis % 60000 == 0){
    //heartBeat();
  }
  if (currentMillis % 500 == 0){
    t7showTime();
  }
  if (currentMillis % 600000 == 0)
  {
    OTA_git_CALL();
  }
  /*
  if (currentMillis % 120000 == 0)
  {
    heartBeat();
  }
  */
  client.loop();
  
}

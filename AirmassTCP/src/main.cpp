//#include "HardwareSerial_NB_BC95.h"
#include <Adafruit_MLX90614.h>

#include <WiFi.h>
#include <DNSServer.h>
#include <WiFiManager.h>
#include <WiFiClientSecure.h>
//#include <TaskScheduler.h>
#include <PubSubClient.h>
#include <TFT_eSPI.h>
#include <SPI.h>
//#include <BME280I2C.h>
#include <SparkFunBME280.h>
#include <Wire.h>

//#include "Adafruit_SGP30.h"
#include <SparkFun_SGP30_Arduino_Library.h>

//#include "Logo.h"
#include "LogoAIS.h"
#include "lv1.h"
#include "lv2.h"
#include "lv3.h"
#include "lv4.h"
#include "lv5.h"
#include "lv6.h"
#include "Splash2.h"
//#include "NBIOT.h"
#include "wifilogo.h"

#include "Free_Fonts.h"
#include "EEPROM.h"
#include <ESPmDNS.h>
#include <ESPUI.h>
#include "http_ota.h"
#include <Arduino.h>

//EEPROMClass  TVOCBASELINE("eeprom1", 0x200);
//EEPROMClass  eCO2BASELINE("eeprom2", 0x100);

void drawUpdate(int num, int x, int y);
void t7showTime();
void _initSGP30 ();
void splash();
void getDataSGP30();
void drawPM2_5(int num, int x, int y);
void drawPM1(int num, int x, int y);
void drawCO2(int num, int x, int y);
void drawPM10(int num, int x, int y);
void drawH(int num, int x, int y);
void drawT(int num, int x, int y);
void drawVOC(int num, int x, int y);
void enterDetailsCallback(Control *sender, int type);

#define _TASK_SLEEP_ON_IDLE_RUN
#define _TASK_PRIORITY
#define _TASK_WDT_IDS
#define _TASK_TIMECRITICAL

#define WIFI_AP ""
#define WIFI_PASSWORD ""


#define CF_OL24 &Orbitron_Light_24
#define CF_OL32 &Orbitron_Light_32

#define title1 "PM2.5" // Text that will be printed on screen in any font
#define title2 "PM1"
#define title3 "PM10"
#define title4 "Co2"
#define title5 "VOC"
#define FILLCOLOR1 0xFFFF

#define TFT_BURGUNDY  0xF1EE


String hostUI = "AirMassTCP: ";

///UI handles
uint16_t wifi_ssid_text, wifi_pass_text;
uint16_t nameLabel, idLabel, cuationlabel, firmwarelabel, mainSwitcher, mainSlider, mainText, settingZNumber, resultButton, mainTime, downloadButton, selectDownload, logStatus;
//  uint16_t styleButton, styleLabel, styleSwitcher, styleSlider, styleButton2, styleLabel2, styleSlider2;
uint16_t tempText, humText, humText2, saveConfigButton, interval ,emailText1;
uint16_t pm01Text, pm25Text, pm10Text, pn03Text, pn05Text, pn10Text, pn25Text, pn50Text, pn100Text, lineText;
uint16_t bmeLog, wifiLog, teleLog;

int periodSendTelemetry = 60;  //the value is a number of seconds

int TempOffset, HumOffset1, pm01Offset, pm25Offset, pm10Offset, pn03Offset, pn05Offset, pn10Offset, pn25Offset, pn50Offset, pn100Offset, CO2Offset, VOCOffset;
String bmeStatus, mqttStatus;

int xpos = 0;
int ypos = 0;

boolean ready2display = false;

int testNum = 0;
int wtd = 0;
int maxwtd = 10;

int tftMax = 160;

int countC = 0;

String nccidStr = "";
String DeTokenStr = "";

int Reset = 0;
Adafruit_MLX90614 mlx = Adafruit_MLX90614();

int error;

//Signal meta ;
String json = "";
String attr = "";

HardwareSerial hwSerial(2);
#define SERIAL1_RXPIN 25
#define SERIAL1_TXPIN 26
//BME280I2C bme;
BME280 bme;

String deviceToken = "";
String serverIP = "147.50.151.130"; // Your Server IP;
String serverPort = "19956";

WiFiClientSecure wifiClient;
PubSubClient client(wifiClient);

//HardwareSerial_NB_BC95 AISnb;

float temp(NAN), hum(NAN), pres(NAN);

#include <TimeLib.h>
#include <ArduinoJson.h>
#include "time.h"
#include <ArduinoOTA.h>

String HOSTNAME = "AIRMASS2.5 : ";
#define PASSWORD "green7650"

const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 3600 * 7;

int nbErrorTime = 0;
bool connectWifi = false;
StaticJsonDocument<400> doc;
bool validEpoc = false;
unsigned long time_s = 0;
unsigned long _epoch = 0;
struct tm timeinfo;
WiFiManager wifiManager;
//const boolean isCALIBRATESGP30 = false;

String imsi = "";
String NCCID = "";
boolean readPMS = false;
TFT_eSPI tft = TFT_eSPI();

TFT_eSprite stringPM25 = TFT_eSprite(&tft);
TFT_eSprite stringPM1 = TFT_eSprite(&tft);
TFT_eSprite stringPM10 = TFT_eSprite(&tft);
TFT_eSprite stringCO2 = TFT_eSprite(&tft);
TFT_eSprite stringVOC = TFT_eSprite(&tft);
TFT_eSprite stringUpdate = TFT_eSprite(&tft);

TFT_eSprite topNumber = TFT_eSprite(&tft);
TFT_eSprite ind = TFT_eSprite(&tft);
TFT_eSprite H = TFT_eSprite(&tft);
TFT_eSprite T = TFT_eSprite(&tft);

int status = WL_IDLE_STATUS;
String downlink = "";
char *bString;
int PORT = 8883;

struct pms7003data {
  uint16_t framelen;
  uint16_t pm10_standard, pm25_standard, pm100_standard;
  uint16_t pm01_env, pm25_env, pm100_env;
  uint16_t particles_03um, particles_05um, particles_10um, particles_25um, particles_50um, particles_100um;
  uint16_t unused;
  uint16_t checksum;
};

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

void setupWIFI()
{
  WiFi.setHostname(HOSTNAME.c_str());

  //等待5000ms，如果没有连接上，就继续往下
  //不然基本功能不可用
  byte count = 0;
  while (WiFi.status() != WL_CONNECTED && count < 10)
  {
    count ++;
    delay(500);
    Serial.print(".");
  }


  if (WiFi.status() == WL_CONNECTED)
    Serial.println("Connecting...OK.");
  else
    Serial.println("Connecting...Failed");
}

void writeString(char add, String data)
{
  int _size = data.length();
  int i;
  for (i = 0; i < _size; i++)
  {
    EEPROM.write(add + i, data[i]);
  }
  EEPROM.write(add + _size, '\0'); //Add termination null character for String Data
  EEPROM.commit();
}

void _writeEEPROM(String data) {
  //Serial.print("Writing Data:");
  //Serial.println(data);
  writeString(10, data);  //Address 10 and String type data
  delay(10);
}

String read_String(char add)
{
  int i;
  char data[100]; //Max 100 Bytes
  int len = 0;
  unsigned char k;
  k = EEPROM.read(add);
  while (k != '\0' && len < 500) //Read until null character
  {
    k = EEPROM.read(add + len);
    data[len] = k;
    len++;
  }
  data[len] = '\0';

  return String(data);
}

//Adafruit_SGP30 sgp;
SGP30 sgp;

//uint32_t getAbsoluteHumidity(float temperature, float humidity) {
  // approximation formula from Sensirion SGP30 Driver Integration chapter 3.15
//  const float absoluteHumidity = 216.7f * ((humidity / 100.0f) * 6.112f * exp((17.62f * temperature) / (243.12f + temperature)) / (273.15f + temperature)); // [g/m^3]
//  const uint32_t absoluteHumidityScaled = static_cast<uint32_t>(1000.0f * absoluteHumidity); // [mg/m^3]
//  return absoluteHumidityScaled;
//}

#define tvoc_topic "sensor/tvoc"
#define eco2_topic "sensor/eco2"

void t1CallGetProbe();
void t2CallShowEnv();
void t3CallSendData();
void t4CallPrintPMS7003();
//void t5CallSendAttribute();
//void t8OTA_git_CALL();


struct pms7003data data;
uint16_t pm01_env = 0;
uint16_t pm25_env = 0;
uint16_t pm100_env = 0;
uint16_t TVOC = 0;
uint16_t eCO2 = 0;
float temp2 = 0;
float hum2 = 0;
void configModeCallback (WiFiManager *myWiFiManager) {
  Serial.println("Entered config mode");
  Serial.println(WiFi.softAPIP());
  //if you used auto generated SSID, print it
  Serial.println(myWiFiManager->getConfigPortalSSID());
}


void _initLCD() {

  tft.fillScreen(TFT_BLACK);
  // TFT
  splash();
  // MLX
  mlx.begin();
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

void _initBME280()
{

  while (!Serial) {} // Wait

  delay(200);
  
  Wire.begin(21, 22);
  //Wire.begin();
  //Sensor supports I2C speeds up to 400kHz
  //Wire.setClock(400000);
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

/*
void initBaseLine() {
  //  Serial.println("Testing EEPROMClass\n");

  if (!TVOCBASELINE.begin(TVOCBASELINE.length())) {
    //    Serial.println("Failed to initialise eCO2BASELINE");
    //    Serial.println("Restarting...");
    delay(1000);
    ESP.restart();
  }
  if (!eCO2BASELINE.begin(eCO2BASELINE.length())) {
    Serial.println("Failed to initialise eCO2BASELINE");
    Serial.println("Restarting...");
    delay(1000);
    ESP.restart();
  }

}
*/

void errorTimeDisplay(int i) {
  tft.fillScreen(TFT_WHITE);
  int xpos = tft.width() / 2; // Half the screen width
  int ypos = tft.height() / 2;
  tft.drawString("Connect NB failed " + String(i + 1) + " times", xpos, ypos, GFXFF);
}

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
  ypos = 180;
  tft.drawString("AIRMASS2.5 Inspector", xpos, ypos, GFXFF);  // Draw the text string in the selected GFX free font
  //AISnb.debug = true;
  //AISnb.setupDevice(serverPort);
  //

  //imsi = AISnb.getIMSI();
  //NCCID = AISnb.getNCCID();
  //imsi.trim();
  //NCCID.trim();
  deviceToken.trim();

  //nccidStr.concat("NCCID:");
  //nccidStr.concat(NCCID);
  DeTokenStr.concat("deviceID: ");
  DeTokenStr.concat(deviceToken);
  //String imsiStr = "";
  //imsiStr.concat("IMSI:");
  //imsiStr.concat(imsi);
  delay(4000);
  tft.setTextColor(TFT_WHITE);
  tft.setFreeFont(FSB9);
  tft.drawString(DeTokenStr, xpos, ypos + 20, GFXFF);
  //tft.drawString(imsiStr, xpos, ypos + 40, GFXFF);
  Serial.println(deviceToken);
  delay(5000);

  tft.setTextFont(GLCD);
  tft.setRotation(1);
  tft.fillScreen(TFT_WHITE);
  // Select the font
  ypos += tft.fontHeight(GFXFF);                      // Get the font height and move ypos down
  tft.setFreeFont(FSB9);
  tft.pushImage(tft.width() / 2 - (Splash2Width / 2) - 15, 3, Splash2Width, Splash2Height, Splash2);



  delay(1200);
  tft.setTextPadding(180);
  tft.setTextColor(TFT_GREEN);
  tft.setTextDatum(MC_DATUM);
  Serial.println(F("Start..."));
  for ( int i = 0; i < 170; i++)
  {
    tft.drawString(".", 1 + 2 * i, 210, GFXFF);
    delay(10);
    Serial.println(i);
  }
  Serial.println("end");
}

void printBME280Data()
{
  Serial.println("DebugprintStartBME280Data");
  //_initBME280();
  //  BME280::TempUnit tempUnit(BME280::TempUnit_Celsius);
  //  BME280::PresUnit presUnit(BME280::PresUnit_Pa);
  //  bme.read(pres, temp, hum, tempUnit, presUnit);

  temp2 = bme.readTempC();  //compensate

  hum2 = bme.readFloatHumidity();
  pres = bme.readFloatPressure();
  Serial.printf("Temp: %.2f, Hum: %.2f, Pres:: %.2f \n", temp2, hum2, pres);
  Serial.println("DebugprintEndBME280Data");
}

void composeJson() {
  //meta = AISnb.getSignal();
  json = "";
  json.concat(" {\"Tn\":\"");
  json.concat(deviceToken);
  json.concat("\",\"temp\":");
  json.concat(temp);
  json.concat(",\"hum\":");
  json.concat(hum);
  json.concat(",\"pres\":");
  json.concat(pres);
  json.concat(",\"pm1\":");
  json.concat(data.pm01_env);
  json.concat(",\"pm2.5\":");
  json.concat(data.pm25_env);
  json.concat(",\"pm10\":");
  json.concat(data.pm100_env);

  json.concat(",\"pn03\":");
  json.concat(data.particles_03um);
  json.concat(",\"pn05\":");
  json.concat(data.particles_05um);
  json.concat(",\"pn10\":");
  json.concat(data.particles_10um);
  json.concat(",\"pn25\":");
  json.concat(data.particles_25um);
  json.concat(",\"pn50\":");
  json.concat(data.particles_50um);
  json.concat(",\"pn100\":");
  json.concat(data.particles_100um);
  json.concat(",\"co2\":");
  json.concat(sgp.CO2);
  json.concat(",\"voc\":");
  json.concat(sgp.TVOC);

  json.concat(",\"rssi\":");
  //if (connectWifi == false) {
  //  json.concat(meta.rssi);
  //} else {
  //  json.concat(WiFi.RSSI());
  //}
  json.concat("}");
  Serial.println(json);
  //SerialBT.println(json);
  if (data.pm25_env > 1000)
    ESP.restart();

}

void t4CallPrintPMS7003() {

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

}

void t2CallShowEnv() {
  Serial.print("Count : ");
  Serial.println(countC);
  //  Serial.print(F("ready2display:"));
  //  Serial.println(ready2display);
  if (ready2display) {

    tft.setTextDatum(MC_DATUM);
    xpos = tft.width() / 2 ; // Half the screen width

    tft.setTextColor(TFT_WHITE);
    tft.setTextSize(1);
    tft.setFreeFont(CF_OL32);
    int mid = (tftMax / 2) - 72;
    tft.setTextPadding(100);
    tft.drawString(title1, xpos - 70, 135, GFXFF); // Print the test text in the custom font

    //  for testing
    //        data.pm25_env = testNum;    //for testing
    //        testNum++;
    //drawNumberParticules();
    drawPM2_5((pm25_env / countC), mid, 50);

    tft.setTextSize(1);
    tft.setFreeFont(CF_OL32);                 // Select the font

    tft.setTextDatum(BR_DATUM);
    tft.setTextColor(TFT_WHITE);
    tft.setTextSize(1);

    tft.setFreeFont(FSB9);   // Select Free Serif 9 point font, could use:

    drawPM1((pm01_env / countC), 6, 195);
    tft.drawString(title2, 40, 235, GFXFF); // Print the test text in the custom font

    drawPM10((pm100_env / countC), 65, 195);
    tft.drawString(title3, 110, 235, GFXFF); // Print the test text in the custom font

    drawCO2((sgp.CO2), 130, 195);
    tft.drawString(title4, 160, 235, GFXFF); // Print the test text in the custom font

    drawVOC((sgp.TVOC), 185, 195);
    tft.drawString(title5, 225, 235, GFXFF); // Print the test text in the custom font

    tft.drawString("RH ", xpos + 115, 214, GFXFF); // Print the test text in the custom font
    drawH((hum2), xpos + 121, 195);
    tft.drawString("%", xpos + 154, 214, GFXFF);

    tft.drawString("T ", xpos + 115, 235, GFXFF); // Print the test text in the custom font
    drawT((temp2), xpos + 121, 214);
    tft.drawString("C", xpos + 153, 235, GFXFF);

    //Clear Stage
    pm01_env = 0;
    pm25_env = 0;
    pm100_env = 0;
    countC = 0;
    TVOC = 0;
    eCO2 = 0;
    temp2 = 0;
    hum2 = 0;
    ind.createSprite(320, 10);
    ind.fillSprite(TFT_BLACK);
    if ((data.pm25_env >= 0) && (data.pm25_env <= 15.4)) {
      tft.setWindow(0, 25, 55, 55);
      tft.pushImage(tft.width() - lv1Width - 6, 45, lv1Width, lv1Height, lv1);
      ind.fillTriangle(0, 0, 5, 5, 10, 0, FILLCOLOR1);
    } else if ((data.pm25_env >= 15.5) && (data.pm25_env <= 40.4)  ) {
      tft.pushImage(tft.width() - lv2Width - 6, 45, lv2Width, lv2Height, lv2);
      ind.fillTriangle(55, 0, 60, 5, 65, 0, FILLCOLOR1);
    } else  if ((data.pm25_env >= 40.5) && (data.pm25_env <= 65.4)  ) {
      tft.pushImage(tft.width() - lv3Width - 6, 45, lv3Width, lv3Height, lv3);
      ind.fillTriangle(105, 0, 110, 5, 115, 0, FILLCOLOR1);
    } else  if ((data.pm25_env >= 65.5) && (data.pm25_env <= 150.4)  ) {
      tft.pushImage(tft.width() - lv4Width - 6, 45, lv4Width, lv4Height, lv4);
      ind.fillTriangle(155, 0, 160, 5, 165, 0, FILLCOLOR1);
    } else  if ((data.pm25_env >= 150.5) && (data.pm25_env <= 250.4)  ) {
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

void drawNumberParticules() {

  topNumber.createSprite(280, 40);
  //  stringPM1.fillSprite(TFT_GREEN);
  topNumber.setFreeFont(FS9);
  topNumber.setTextColor(TFT_WHITE);
  topNumber.setTextSize(1);           // Font size scaling is x1

  topNumber.drawString(">1.0um", 0, 21, GFXFF); // Print the test text in the custom font
  topNumber.drawNumber(data.particles_10um, 10, 0);   //tft.drawString("0.1L air", 155, 5, GFXFF);
  topNumber.drawString(">2.5um", 95, 21, GFXFF); // Print the test text in the custom font
  topNumber.drawNumber(data.particles_25um, 105, 0);   //tft.drawString("0.1L air", 155, 5, GFXFF);
  topNumber.drawString(">5.0um", 180, 21, GFXFF); // Print the test text in the custom font
  topNumber.drawNumber(data.particles_50um, 192, 0);   //tft.drawString("0.1L air", 155, 5, GFXFF);

  topNumber.pushSprite(5, 5);
  topNumber.deleteSprite();
}

boolean readPMSdata(Stream *s) {
  //  Serial.println("readPMSdata");
  if (! s->available()) {
    Serial.println("readPMSdata.false");
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
  pm01_env += data.pm01_env;
  pm25_env += data.pm25_env;
  pm100_env += data.pm100_env;
  if (sum != data.checksum) {
    Serial.println("Checksum failure");
    return false;
  }
  // success!
  return true;
}

String a0(int n) {
  return (n < 10) ? "0" + String(n) : String(n);
}

struct tm tmstruct;
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
  configTime(3600 * timezone, daysavetime * 3600, "time.nist.gov", "0.pool.ntp.org", "1.pool.ntp.org");
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
    if (!getLocalTime(&tmstruct)) {
      Serial.println("Failed to obtain time");
      //      ESP.restart();
      return;
    }
  }

  topNumber.drawString(timeS, 5, 10, GFXFF);

  topNumber.pushSprite(5, 5);
  topNumber.deleteSprite();


}

void t1CallGetProbe() {
  //tCallback();
  boolean pmsReady = readPMSdata(&hwSerial);

  if ( pmsReady ) {
    ready2display = true;
    wtd = 0;
  } else {
    ready2display = false;

  }


  if (wtd > maxwtd)
    ESP.restart();

  printBME280Data();
  getDataSGP30();
  countC += 1;
}

void drawUpdate(int num, int x, int y)
{
  stringUpdate.createSprite(60, 20);
  stringUpdate.fillScreen(TFT_BLACK);
  stringUpdate.setFreeFont(FSB9);
  stringUpdate.setTextColor(TFT_ORANGE);
  stringUpdate.setTextSize(1);
  stringUpdate.drawNumber(num, 0, 3);
  stringUpdate.drawString("%", 25, 3, GFXFF);
  stringUpdate.pushSprite(x, y);
  stringUpdate.deleteSprite();
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

void drawT(int num, int x, int y)
{
  T.createSprite(50, 20);
  //  stringPM1.fillSprite(TFT_GREEN);
  T.setFreeFont(FSB9);
  T.setTextColor(TFT_WHITE);
  T.setTextSize(1);
  T.drawNumber(num, 0, 3);
  T.pushSprite(x, y);
  T.deleteSprite();
}

void drawH(int num, int x, int y)
{
  H.createSprite(50, 20);
  //  stringPM1.fillSprite(TFT_GREEN);
  H.setFreeFont(FSB9);
  H.setTextColor(TFT_WHITE);
  H.setTextSize(1);
  H.drawNumber(num, 0, 3);
  H.pushSprite(x, y);
  H.deleteSprite();
}


void drawPM1(int num, int x, int y)
{
  stringPM1.createSprite(50, 20);
  //  stringPM1.fillSprite(TFT_GREEN);
  stringPM1.setFreeFont(FSB9);
  stringPM1.setTextColor(TFT_WHITE);
  stringPM1.setTextSize(1);
  stringPM1.drawNumber(num, 0, 3);
  stringPM1.pushSprite(x, y);
  stringPM1.deleteSprite();
}

void drawCO2(int num, int x, int y)
{
  stringCO2.createSprite(60, 20);
  //  stringCO2.fillSprite(TFT_GREEN);
  stringCO2.setFreeFont(FSB9);
  stringCO2.setTextColor(TFT_WHITE);
  stringCO2.setTextSize(1);
  stringCO2.drawNumber(num, 0, 3);
  stringCO2.pushSprite(x, y);
  stringCO2.deleteSprite();
}

void drawVOC(int num, int x, int y)
{
  stringVOC.createSprite(60, 20);
  //  stringVOC.fillSprite(TFT_GREEN);
  stringVOC.setFreeFont(FSB9);
  stringVOC.setTextColor(TFT_WHITE);
  stringVOC.setTextSize(1);
  stringVOC.drawNumber(num, 0, 3);
  stringVOC.pushSprite(x, y);
  stringVOC.deleteSprite();
}
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

void getDataSGP30() {
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
  //ESPUI.updateLabel(idLabel, String(deviceToken));
}

void _initUI(){
  Serial.println("Start Debug init UI");
  hostUI = "SmartPeir:" + deviceToken;
#if defined(ESP32)
  WiFi.setHostname(hostUI.c_str());
#else
  WiFi.hostname(hostUI.c_str());
#endif
  //host2 = "SmartPeir";
    //if (WiFi.status() == WL_CONNECTED) {
    //  Serial.println(WiFi.localIP());
    //  Serial.println("Wifi started");

    //  if (!MDNS.begin(hostUI.c_str())) {
    //    Serial.println("Error setting up MDNS responder!");
    //  }
    //} else {
		Serial.println("\nCreating access point...");
    Serial.println(WiFi.localIP());
		WiFi.mode(WIFI_MODE_STA); // Correct
		WiFi.softAPConfig(IPAddress(192, 168, 1, 1), IPAddress(192, 168, 1, 1), IPAddress(255, 255, 255, 0));
		WiFi.softAP(hostUI.c_str());
  //}
  Serial.println("End Debug init UI");
  
}

// This is the main function which builds our GUI
void setUpUI() {
 // Serial.println("Start Debug UI");

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
  
  //ESPUI.addControl(Separator, "Email Registry", "", None, settingTab);
  /**/
  //emailText1 = ESPUI.addControl(Text, "Email User 1", "", Emerald, settingTab, enterDetailsCallback);
  //lineText = ESPUI.addControl(Text, "Line ID", "", Emerald, settingTab, enterDetailsCallback);

  /*
  emailText2 = ESPUI.addControl(Text, "Email User 2", email.email2, Emerald, settingTab, enterDetailsCallback);
  emailText3 = ESPUI.addControl(Text, "Email User 3", email.email3, Emerald, settingTab, enterDetailsCallback);
  emailText4 = ESPUI.addControl(Text, "Email User 4", email.email4, Emerald, settingTab, enterDetailsCallback);
  emailText5 = ESPUI.addControl(Text, "Email User 5", email.email5, Emerald, settingTab, enterDetailsCallback);
  /**/

  ESPUI.addControl(Separator, "", "", None, settingTab);
  //  ESPUI.addControl(Button, "Refresh", "Refresh", Peterriver, settingTab, enterDetailsCallback);
  ESPUI.addControl(Button, "Save", "SAVE", Peterriver, settingTab, enterDetailsCallback);
  
  auto eventTab = ESPUI.addControl(Tab, "", "Event Log");
  ESPUI.addControl(Separator, "Error Log", "", None, eventTab);
  teleLog = ESPUI.addControl(Label, "Server Connection Status", String(mqttStatus), Alizarin, eventTab, enterDetailsCallback);
  bmeLog = ESPUI.addControl(Label, "Sensor Connection Status", String(bmeStatus), Alizarin, eventTab, enterDetailsCallback);
  hostUI = "AirMassTCP:" + deviceToken;
  //Finally, start up the UI.
  //This should only be called once we are connected to WiFi.
  Serial.println("Starting ESPUI...");
  ESPUI.begin(hostUI.c_str());
  Serial.println("ESPUI started.");
  
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
    //Control* email1_ = ESPUI.getControl(emailText1);
    //Control* lineID_ = ESPUI.getControl(lineText);
    
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
    //email1 = email1_->value;
    //lineID = lineID_->value;
    char data1[40];
    char data2[40];
    char data3[40];
    //email1.toCharArray(data1, 40);  // Convert String to char array
    //lineID.toCharArray(data2, 40);
    deviceToken.toCharArray(data3, 40);
    
    // Print to Serial
    Serial.println("put TempOffset: " + String(TempOffset));
    Serial.println("put HumOffset1: " + String(HumOffset1));
    Serial.println("put periodSendTelemetry: " + String(periodSendTelemetry));
    //Serial.println("put email1: " + String(email1));


    // Write to EEPROM
    //EEPROM.begin(180); // Ensure enough size for data
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
    //addr = 70;
    //for (int len = 0; len < email1.length(); len++) {
    //  EEPROM.write(addr + len, data1[len]);  // Write each character
    //}
    //EEPROM.write(addr + email1.length(), '\0');  // Add null terminator at the end
    //addr = 110;
    //for (int len = 0; len < lineID.length(); len++) {
    //  EEPROM.write(addr + len, data2[len]);  // Write each character
    //}
    //EEPROM.write(addr + lineID.length(), '\0');  // Add null terminator at the end
    
    //addr = 140;
    for (int len = 0; len < deviceToken.length(); len++) {
      EEPROM.write(addr + len, data3[len]);  // Write each character
    }
    EEPROM.write(addr + deviceToken.length(), '\0');  // Add null terminator at the end
    
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
    //EEPROM.end();
    //sendAttribute(); // Assuming this function is required to send attributes
  }
}

void readEEPROM() {
  Serial.println("Reading credentials from EEPROM...");
  //EEPROM.begin(180); // Ensure enough size for data
  
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
    //addr = 70;
    //for (int len = 0; len < 50; len++){
    //  char data1 = EEPROM.read(addr + len);
    //  if(data1 == '\0' || data1 == 255) break;
    //  email1 += data1;
    //}
    //  addr += sizeof(email1);
    //addr = 110;
    //for (int len = 0; len < 50; len++){
    //  char data2 = EEPROM.read(addr + len);
    //  if(data2 == '\0' || data2 == 255) break;
    //  lineID += data2;
    //}
    
    //addr = 140;
    for (int len = 0; len < 50; len++){
      char data3 = EEPROM.read(addr + len);
      if(data3 == '\0' || data3 == 255) break;
      deviceToken += data3;
    }
    
    //EEPROM.end();
  // Print to Serial
  Serial.println("get TempOffset: " + String(TempOffset));
  Serial.println("get HumOffset1: " + String(HumOffset1));
  Serial.println("get periodSendTelemetry: " + String(periodSendTelemetry));
  //Serial.println("get outputEmail1: " + String(email1));
  if (periodSendTelemetry == 225, -1){
    pm01Offset = 0;
    pm25Offset = 0;
    pm10Offset = 0;
    pn03Offset = 0;
    pn05Offset = 0;
    pn10Offset = 0;
    pn25Offset = 0;
    pn50Offset = 0;
    pn100Offset = 0;
    periodSendTelemetry = 60;
  }
  

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
  //ESPUI.updateText(emailText1, String(email1));
  //ESPUI.updateText(lineText, String(lineID));
}

void setup() {
  Serial.begin(115200);
  Project = "SmartPier";
  FirmwareVer = 0.2;

  delay(500);
  pinMode(15, OUTPUT);
  digitalWrite(15, HIGH);  
  pinMode(32, OUTPUT); // on BME280
  digitalWrite(32, HIGH); // on BME280
  pinMode(33, OUTPUT); // turn on PMS7003
  digitalWrite(33, HIGH); // turn on PMS7003
  delay(500);

  EEPROM.begin(512);
  readEEPROM();
  delay(1000);
  _initLCD();

  tft.fillScreen(TFT_WHITE);
  tft.drawString("Wait for WiFi Setting (Timeout 60 Sec)", tft.width() / 2, tft.height() / 2, GFXFF);
  wifiManager.setTimeout(60);

  wifiManager.setAPCallback(configModeCallback);
  HOSTNAME.concat(String((uint32_t)ESP.getEfuseMac(), HEX));
  if (!wifiManager.autoConnect(HOSTNAME.c_str())) {
    //Serial.println("failed to connect and hit timeout");
    //reset and try again, or maybe put it to deep sleep
    //    ESP.reset();
    //delay(1000);
  }
  getMac();
  if (nbErrorTime == 10) {
    connectWifi = true;
  }
  if (connectWifi == false) {
    json = "{\"_type\":\"retrattr\",\"Tn\":\"";
    json.concat(deviceToken);
    json.concat("\"]}");
  }
  //if (connectWifi == true) {
  //  configTime(gmtOffset_sec, 0, ntpServer);
  //  client.setServer( "mqtt.thingcontrol.io", PORT );
  //}

  setupWIFI();
  //setupOTA();

  hwSerial.begin(9600, SERIAL_8N1, SERIAL1_RXPIN, SERIAL1_TXPIN);
  _initBME280();

  _initSGP30();

  configTime(3600 * timezone, daysavetime * 3600, "0.pool.ntp.org", "1.pool.ntp.org", "time.nist.gov");
  //runner.init();
  //  Serial.println("Initialized scheduler");

  //runner.addTask(t1);
  //  Serial.println("added t1");
  //runner.addTask(t2);
  //  Serial.println("added t2");
  //runner.addTask(t3);
  //  Serial.println("added t3");
  //runner.addTask(t4);
  //  Serial.println("added t4");
  //runner.addTask(t5);
  //  Serial.println("added t5");
  //runner.addTask(t7);
  //runner.addTask(t8);

  //delay(2000);

  //t1.enable();
  //  Serial.println("Enabled t1");
  //t2.enable();
  //  Serial.println("Enabled t2");
  //t3.enable();
  //  Serial.println("Enabled t3");
  //t4.enable();
  //t5.enable();
  //t7.enable();
  //t8.enable();
  //Serial.println("Debug setup 1");
  hostUI = "AirMassTCP:" + deviceToken;
  MDNS.begin(hostUI.c_str());
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAPConfig(IPAddress(192, 168, 1, 1), IPAddress(192, 168, 1, 1), IPAddress(255, 255, 255, 0));
  WiFi.softAP(hostUI.c_str());
  setUpUI();
  for (int i = 0; i < 1000; i++);
  tft.fillScreen(TFT_BLACK);            // Clear screen
  tft.fillRect(5, 185, tft.width() - 15, 5, TFT_GREEN); // Print the test text in the custom font
  tft.fillRect(63, 185, tft.width() - 15, 5, TFT_YELLOW); // Print the test text in the custom font
  tft.fillRect(113, 185, tft.width() - 15, 5, TFT_ORANGE); // Print the test text in the custom font
  tft.fillRect(166, 185, tft.width() - 15, 5, TFT_RED); // Print the test text in the custom font
  tft.fillRect(219, 185, tft.width() - 15, 5, TFT_PURPLE); // Print the test text in the custom font
  tft.fillRect(272, 185, tft.width() - 15, 5, TFT_BURGUNDY); // Print the test text in the custom font
  //Serial.println("Scheduler Priority Test");
  //delay(2000);
  
  //delay(5000);

}

void loop() {
  //ArduinoOTA.handle();
  //runner.execute();
  const unsigned long currentMillis = millis();
  const unsigned long time2send = periodSendTelemetry * 1000;
  if (currentMillis % time2send == 0){
    t3CallSendData();
    OTA_git_CALL();
  }
  if (currentMillis % 2000 == 0){
    t1CallGetProbe();
    t2CallShowEnv();
    t4CallPrintPMS7003();
  }
  if(currentMillis % 500 == 0){
    t7showTime();
  }
  //if (currentMillis % 120000 == 0)
}
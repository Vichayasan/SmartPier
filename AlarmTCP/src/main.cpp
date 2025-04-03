#include <WiFiManager.h>
#include <SPI.h>
#include <ArduinoJson.h>
#include <EEPROM.h>
#include <ESPUI.h>
#include "http_ota.h"
#include <ESPmDNS.h>
#include <Adafruit_MCP23008.h>
#include <ESPAsyncWebServer.h>
#include <ESPmDNS.h>
#include <HTTPClient.h>
#include <ModbusServerWiFi.h>

WiFiManager wifiManager;
WiFiClient wifiClient;
Adafruit_MCP23008 mcp;
ModbusServerWiFi mb;

String host = "WifiManager:";
String deviceToken = "";
const char* deviceID;
const char* nameID;
const char* version;
String ServAddress = "192.168.0.1";
String localIP = "";
String hostUI;
int ClAddr;
// float value = 0;
int periodSendTelemetry = 30;
int TCPaddress = 3;
int periodOTA = 60;
char h1[10], h2[10], h3[10]; 
float avgh;
int immersH, totalH, immersW;
int lenght, width;
int Density = 1000;
//float Gravity = 981;
int greenW, yellowW; //relay3,  relay2
int greenH, yellowH ,redH;
unsigned long previousMillis = 0;
unsigned long readmillis = 0;
unsigned long interupmillis = 0;
int ar[3] = {0};
int butPin = 25;
int recon = 0;
String url[3] = {""};
uint16_t msg[6] = {0};
int rssi = 0;

String wifistat, httpstat;

uint16_t nameLabel, idLabel, firmwarelabel, myIP;
uint16_t clientAddr, serverIP;
uint16_t interval, otaTime, hTotal, dens, grav, x, y;
uint16_t greenAW, yellowAW, greenAH, yellowAH;
uint16_t ul1, ul2, ul3;
uint16_t wifiLog, httpbug, URL1, URL2, URL3, sig;

void configModeCallback(WiFiManager *myWiFiManager);
void enterConnectionCallback(Control *sender, int type);
void enterSettingCallback(Control *sender, int type);
void eepromWrite();
void eepromRead();
float avgHeight();
void enterAlarmSetCallback(Control *sender, int type);

struct tcp_pcb;
extern struct tcp_pcb* tcp_tw_pcbs;
extern "C" void tcp_abort(struct tcp_pcb* pcb);

void tcpCleanup(void) {
    while (tcp_tw_pcbs) {
        tcp_abort(tcp_tw_pcbs);
    }
}


void _initWiFiManager()
{
  wifiManager.setAPCallback(configModeCallback);
  wifiManager.setConfigPortalTimeout(60); // auto close configportal after n seconds
  wifiManager.setAPClientCheck(true);      // avoid timeout if client connected to softap
  wifiManager.setBreakAfterConfig(true);   // always exit configportal even if wifi save fails
  host.concat(String((uint32_t)ESP.getEfuseMac(), HEX));
  if (!wifiManager.autoConnect(host.c_str()))
  {
    Serial.println("failed to connect and hit timeout");
    ESP.restart();
    delay(1000);
  }
}

void configModeCallback(WiFiManager *myWiFiManager)
{
  Serial.println("Entered config mode");
  Serial.println(WiFi.softAPIP());
  // if you used auto generated SSID, print it
  Serial.println(myWiFiManager->getConfigPortalSSID());
}

void printWifiStatus() 
{
  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print your WiFi shield's IP address:
  //IPAddress ip = WiFi.localIP();
  localIP = WiFi.localIP().toString();
  Serial.print("IP Address: ");
  Serial.println(localIP);

  // print the received signal strength:
  rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
  ESPUI.updateLabel(myIP, String(localIP));
  ESPUI.updateLabel(sig, String(rssi));
}

float avgHeight()
{
  int H1 = ar[0];
  int H2 = ar[1];
  int H3 = ar[2];
  Serial.printf("H1: %d, H2: %d, H3: %d\n", H1, H2 ,H3);

  avgh = (H1 + H2 + H3) / 3;
  msg[1] = avgh;
  Serial.printf("Average Height: %2f\n", avgh);
  Serial.printf("msg: %d\n", msg[0]);

  return avgh;
}

float calHeight()
{
  immersH = (totalH - avgh) * 0.01;
  Serial.printf("Immersed Height: %2f\n", immersH);
  return immersH;
}

float calBuoyancy()
{
  immersW = Density * (lenght * width * immersH) * 0.0001;
  Serial.printf("Immersed Weight: %2f\n", immersW); //kg
  return immersW;
}

void EnableAlarm()
{
  
  if ( avgh >= greenH){
    mcp.digitalWrite(0, LOW);
    mcp.digitalWrite(1, LOW);
    mcp.digitalWrite(2, HIGH);
    mcp.digitalWrite(3, LOW);
    Serial.printf("avgH: %f, Green: %d\n", avgh, greenH);
  }

  //yellow
  else if ( avgh >= yellowH && avgh <= greenH){
    mcp.digitalWrite(0, LOW);
    mcp.digitalWrite(1, HIGH);
    mcp.digitalWrite(2, LOW);
    mcp.digitalWrite(3, LOW);
    Serial.printf("avgH: %f, yellow: %d\n", avgh, yellowH);
  }

  //red = relay1 pin0
  else{
    mcp.digitalWrite(0, HIGH);
    mcp.digitalWrite(1, LOW);
    mcp.digitalWrite(2, LOW);
    mcp.digitalWrite(3, HIGH);
    Serial.printf("avgH: %f, RED!\n", avgh);
  }
}


void _initUI(){
  hostUI = "AlarmTCP" + deviceToken;
  //host2 = "SmartPeir";
  // MDNS.begin(hostUI.c_str());
  //WiFi.mode(WIFI_MODE_AP);
  WiFi.softAPConfig(IPAddress(192, 168, 1, 1), IPAddress(192, 168, 1, 1), IPAddress(255, 255, 255, 0));
  WiFi.softAP(hostUI.c_str());
  //Finally, start up the UI.
  //This should only be called once we are connected to WiFi.
  
}

void setUpUI() {
  Serial.println("Start UI");

  tcpCleanup();
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
  nameLabel = ESPUI.addControl(Label, "Device Name", String(Project), Peterriver, maintab);
  idLabel = ESPUI.addControl(Label, "Device ID", String(deviceID), Peterriver, maintab);
  firmwarelabel = ESPUI.addControl(Label, "Firmware Version", String(FirmwareVer), Peterriver, maintab);
  myIP = ESPUI.addControl(Label, "IP Address", String(localIP), Peterriver, maintab);

  auto network = ESPUI.addControl(Tab, "", "Connection");
  // serverIP = ESPUI.addControl(Text, "Server IP Adress", String(ServAddress), Peterriver, network, enterConnectionCallback);
  clientAddr = ESPUI.addControl(Number, "Server ID", String(ClAddr), Peterriver, network, enterConnectionCallback);
  ESPUI.addControl(Button, "Save", "SAVE", Dark, network, enterConnectionCallback);

  auto alarmset = ESPUI.addControl(Tab, "", "Alarm");
  ESPUI.addControl(Separator, "Height Configuration","", None, alarmset);
  greenAH = ESPUI.addControl(Number, "Maximum of Green Level(cm)", String(greenH), Peterriver, alarmset, enterAlarmSetCallback);
  yellowAH = ESPUI.addControl(Number, "Maximum of Yellow Level(cm)", String(yellowW), Peterriver, alarmset, enterAlarmSetCallback);
  ESPUI.addControl(Separator, "Weight Configuration","", None, alarmset);
  greenAW = ESPUI.addControl(Number, "Maximum of Green Level(kg)", String(greenW), Peterriver, alarmset, enterAlarmSetCallback);
  yellowAW = ESPUI.addControl(Number, "Maximum of Yellow Level(kg)", String(yellowW), Peterriver, alarmset, enterAlarmSetCallback);
  ESPUI.addControl(Button, "Save", "Save", Dark, alarmset, enterAlarmSetCallback);

  auto setting = ESPUI.addControl(Tab, "", "Setting");
  hTotal = ESPUI.addControl(Number, "Total of Height (cm)", String(totalH), Peterriver, setting, enterSettingCallback);
  dens = ESPUI.addControl(Number, "Density of Liquid (kg/m^3)", String(Density), Peterriver, setting, enterSettingCallback);
  x = ESPUI.addControl(Number, "Lenght(cm)", String(lenght), Peterriver, setting, enterSettingCallback);
  y = ESPUI.addControl(Number, "Width(cm)", String(width), Peterriver, setting, enterSettingCallback);
  ESPUI.addControl(Separator, "Period Configuration","", None, setting);
  interval = ESPUI.addControl(Number, "Telemetry", String(periodSendTelemetry), Peterriver, setting, enterSettingCallback);
  otaTime = ESPUI.addControl(Number, "OTA", String(periodOTA), Peterriver,setting, enterSettingCallback);
  ESPUI.addControl(Button, "Save", "SAVE", Dark, setting, enterSettingCallback);

  auto log = ESPUI.addControl(Tab, "", "Log");
  URL1 = ESPUI.addControl(Label, "Ultrasonic URL 1", String(url[0]), Peterriver, log);
  URL2 = ESPUI.addControl(Label, "Ultrasonic URL 2", String(url[1]), Peterriver, log);
  URL3 = ESPUI.addControl(Label, "Ultrasonic URL 3", String(url[2]), Peterriver, log);
  ESPUI.addControl(Separator, "", "", None, log);
  ul1 = ESPUI.addControl(Label, "Ultrasonic Device 1 (cm)", String(h1), Peterriver, log);
  ul2 = ESPUI.addControl(Label, "Ultrasonic Device 2 (cm)", String(h2), Peterriver, log);
  ul3 = ESPUI.addControl(Label, "Ultrasonic Device 3 (cm)", String(h3), Peterriver, log);

  auto debug = ESPUI.addControl(Tab, "", "Debug");
  ESPUI.addControl(Separator, "Error Log", "", None, debug);
  wifiLog = ESPUI.addControl(Label, "WiFi Connection Status", String(wifistat), Alizarin, debug);
  httpbug = ESPUI.addControl(Label, "Http Client Connection Status", String(httpstat), Alizarin, debug);
  sig = ESPUI.addControl(Label, "RSSI", String(rssi), Alizarin, debug);
  // ESPUI.WebServer()->begin(8000);

  Serial.println("End UI");
  // String hostUI = "AlarmTCP" + deviceToken;
  ESPUI.begin(hostUI.c_str());

}

void enterConnectionCallback(Control *sender, int type) {
  Serial.println(sender->value);
  ESPUI.updateControl(sender);
  if(type == B_UP) {
    Serial.println("Saving Offset to EPROM...");
    // Control* address_ = ESPUI.getControl(serverIP);
    Control* Cl_ = ESPUI.getControl(clientAddr);
    ClAddr = Cl_->value.toInt();
    // ServAddress = address_->value.c_str();
    // ip.fromString(ServAddress);
    eepromWrite();
    
  }
}

void enterSettingCallback(Control *sender, int type){
  Serial.println(sender->value);
  ESPUI.updateControl(sender);
  if(type == B_UP) {
    Serial.println("Saving Offset to EPROM...");
    Control* tele_ = ESPUI.getControl(interval);
    Control* ota_ = ESPUI.getControl(otaTime);
    Control* total_ = ESPUI.getControl(hTotal);
    Control* x_ = ESPUI.getControl(x);
    Control* y_ = ESPUI.getControl(y);
    // Serial.println("Debug Enter Setting 1");
    totalH = total_->value.toInt();
    // Serial.println("Debug Enter Setting 2");
    periodOTA = ota_->value.toInt();
    // Serial.println("Debug Enter Setting 3");
    // Serial.println("Debug Enter Setting 4");
    lenght = x_->value.toInt();
    // Serial.println("Debug Enter Setting 5");
    width = y_->value.toInt();
    // Serial.println("Debug Enter Setting 6");
    periodSendTelemetry = tele_->value.toInt();
    // Serial.println("Debug Enter Setting 7");

    eepromWrite();
    Serial.println("Saved Offset to EPROM...");
  }
}

void enterAlarmSetCallback(Control *sender, int type){
  Serial.println(sender->value);
  ESPUI.updateControl(sender);
  if(type == B_UP){
    Serial.println("Saving Offset to EPROM...");
    Control* grW_ = ESPUI.getControl(greenAW);
    Control* yellW_ = ESPUI.getControl(yellowAW);
    Control* grH_ = ESPUI.getControl(greenAW);
    Control* yellH_ = ESPUI.getControl(yellowAH);
    greenW = grW_->value.toInt();
    yellowW = yellW_->value.toInt();
    greenH = grH_->value.toInt();
    yellowH = yellH_->value.toInt();
    Serial.println("green from UI: " + String(greenH));
    Serial.println("yellow from UI: " + String(yellowH));
   
    eepromWrite();
  }
}

void eepromWrite(){
  Serial.println("Start write to EEPROM");

  char data1[40];
  char data2[40];
  char data3[40];

  // ServAddress.toCharArray(data1, 20);
  
  EEPROM.begin(200);
  int addr = 0;
  
  EEPROM.put(addr, ClAddr);
  addr += sizeof(ClAddr);

  EEPROM.put(addr, periodSendTelemetry);
  addr += sizeof(periodSendTelemetry);

  EEPROM.put(addr, periodOTA);
  addr += sizeof(periodOTA);

  EEPROM.put(addr, totalH);
  addr += sizeof(totalH);

  EEPROM.put(addr, lenght);
  addr += sizeof(lenght);

  EEPROM.put(addr, width);
  addr += sizeof(width);

  EEPROM.put(addr, greenW);
  addr += sizeof(greenW);

  EEPROM.put(addr, yellowW);
  addr += sizeof(yellowW);

  EEPROM.put(addr, greenH);
  addr += sizeof(greenH);

  EEPROM.put(addr, yellowH);
  addr += sizeof(yellowH);

  // for (int len = 0; len < ServAddress.length(); len++) {
  //     EEPROM.write(addr + len, data1[len]);  // Write each character
  //   }
  //   EEPROM.write(addr + ServAddress.length(), '\0');


  EEPROM.end();

  Serial.println("End write to EEPROM");
}

void eepromRead() {
  Serial.println("Reading credentials from EEPROM...");
  EEPROM.begin(200); // Ensure enough size for data
  int addr = 0;

  EEPROM.get(addr, ClAddr);
  addr += sizeof(ClAddr);

  EEPROM.get(addr, periodSendTelemetry);
  addr += sizeof(periodSendTelemetry);

  EEPROM.get(addr, periodOTA);
  addr += sizeof(periodOTA);

  EEPROM.get(addr, totalH);
  addr += sizeof(totalH);

  EEPROM.get(addr, lenght);
  addr += sizeof(lenght);

  EEPROM.get(addr, width);
  addr += sizeof(width);

  EEPROM.get(addr, greenW);
  addr += sizeof(greenW);

  EEPROM.get(addr, yellowW);
  addr += sizeof(yellowW);

  EEPROM.get(addr, greenH);
  addr += sizeof(greenH);

  EEPROM.get(addr, yellowH);
  addr += sizeof(yellowH);

  // for (int len = 0; len < 20; len++) {
  //   char data1 = EEPROM.read(addr + len);
  //   if (data1 == '\0' || data1 == (char)255 || data1 == (char)20) // Skip unwanted characters
  //     break;
  //   ServAddress += data1;
  // }

  EEPROM.end();

  if (periodSendTelemetry == 255 || periodSendTelemetry == -1) {
    periodSendTelemetry = 60;
  }

  // Print to Serial
  Serial.println("----- Read EEPROM Storage value by ESPUI -----");
  Serial.println("Server Address: " + String(ServAddress));
  Serial.printf("periodSendTelemetry: %d\n", periodSendTelemetry);
  Serial.printf("periodOTA: %d\n", periodOTA);
  Serial.printf("Total Height: %d\n", totalH);
  Serial.printf("lenght: %d\n", lenght);
  Serial.printf("width: %d\n", width);
  Serial.printf("DeviceID: %d\n", deviceToken);
  Serial.printf("Geen: %d\n", greenH);

  ESPUI.updateText(serverIP, String(ServAddress));
  ESPUI.updateNumber(clientAddr, ClAddr);
  ESPUI.updateNumber(interval, periodSendTelemetry);
  ESPUI.updateNumber(otaTime, periodOTA);
  ESPUI.updateNumber(hTotal, totalH);
  ESPUI.updateNumber(x, lenght);
  ESPUI.updateNumber(y, width);
  ESPUI.updateNumber(greenAW, greenW);
  ESPUI.updateNumber(yellowAW, yellowW);
  ESPUI.updateNumber(greenAH, greenAH);
  ESPUI.updateNumber(yellowAH, yellowH);
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
  deviceID = deviceToken.c_str();
  ESPUI.updateLabel(idLabel, deviceID);
}

void _initMCP()
{
  mcp.begin();      // use default address 0
  mcp.pinMode(0, OUTPUT);
  mcp.pinMode(1, OUTPUT);
  mcp.pinMode(2, OUTPUT);
  mcp.pinMode(3, OUTPUT);
}

void DisAlarm()
{
  int buttonStat;
  buttonStat = digitalRead(butPin);
  Serial.println("butstat:" + String(buttonStat));
  if (buttonStat == 1){
    mcp.digitalWrite(3, LOW);
    Serial.println("Disable Alarm");
  }
  
}

void log() {
  // // Convert floats in ar[] to strings and copy them to h1, h2, and h3
  // snprintf(h1, sizeof(h1), "%d", (int)ar[1]); // Format ar[1] to 2 decimal places
  // snprintf(h2, sizeof(h2), "%d", (int)ar[2]);
  // snprintf(h3, sizeof(h3), "%d", (int)ar[3]);

  // Update ESPUI labels
  ESPUI.updateLabel(ul1, String(ar[0])); // ESPUI accepts `const char*`, so h1 works
  ESPUI.updateLabel(ul2, String(ar[1]));
  ESPUI.updateLabel(ul3, String(ar[2]));

  Serial.println("debug log");
  Serial.printf("h1: %d, h2: %d, h3: %d\n", ar[0], ar[1], ar[2]); // For debugging
}

// Worker function for serverID=1, function code 0x03 or 0x04
ModbusMessage FC03(ModbusMessage request) {
  uint16_t addr = 0;        // Start address to read
  uint16_t wrds = 5;        // Number of words to read
  ModbusMessage response;
  // Debug: Log the received request
  Serial.println("FC03 called: processing request...");
  Serial.printf("Request Server ID: %d, Function Code: 0x%02X\n", request.getServerID(), request.getFunctionCode());

  // Get addr and words from data array. Values are MSB-first, get() will convert to binary
  request.get(2, addr);
  request.get(4, wrds);
  
  // address valid?
  if (!addr || addr > 128) {
    // No. Return error response
    response.setError(request.getServerID(), request.getFunctionCode(), ILLEGAL_DATA_ADDRESS);
    return response;
  }

  // Modbus address is 1..n, memory address 0..n-1
  addr--;

  // Number of words valid?
  if (!wrds || (addr + wrds) > 127) {
    // No. Return error response
    response.setError(request.getServerID(), request.getFunctionCode(), ILLEGAL_DATA_ADDRESS);
    return response;
  }

  // Prepare response
  response.add(request.getServerID(), request.getFunctionCode(), (uint8_t)(wrds * 2));

  // Loop over all words to be sent
  for (uint16_t i = 0; i < wrds; i++) {
    // Add word MSB-first to response buffer
    response.add(msg[addr + i]);
    Serial.printf("Adding Value to Response: Address %d, Value: %d\n", addr + i + 1, msg[addr + i]);
  }

  // Return the data response
  return response;
}

void setup(){

  Serial.begin(115200);
  Project = "SmartPier";
  FirmwareVer = "0.0.1";

  WiFi.setTxPower(WIFI_POWER_19_5dBm);

  // pinMode(15, OUTPUT);
  // digitalWrite(15, HIGH);
  
  // pinMode(butPin, INPUT);

  // wifiManager.resetSettings();

  Serial.println("WiFi is connecting...");
  _initWiFiManager();
  Serial.println("WiFi is connected!");

  printWifiStatus();

  

  getMac();

  eepromRead();

  _initMCP();

  for (int i = 0; i < 3; i++){
    mcp.digitalWrite(i, HIGH);
    Serial.printf("Test Relay: %d \n", i);
  }
  delayMicroseconds(1000000);
  for (int i = 0; i < 3; i++){
    mcp.digitalWrite(i, LOW);
    Serial.printf("Test Relay: %d \n", i);
  }
  OTA_git_CALL();

  delayMicroseconds(3000000);
  _initUI();
  setUpUI();
  delayMicroseconds(3000000);

  if(mdns_init()!= ESP_OK){
    Serial.println("mDNS failed to start");
    return;
  } else {
    Serial.println("mDNS Started");
  }

  int nrOfServices = MDNS.queryService("http", "tcp");
   
  if (nrOfServices == 0) {
      Serial.println("No services were found.");
  }
  int j = 0;
  
  for (int i = 0; i < nrOfServices; i=i+1) {
   // print service information
   Serial.print("Hostname ");
   Serial.println(i);
   Serial.println(MDNS.hostname(i));
   if (MDNS.hostname(i).indexOf("smartpeir") != -1){
    url[j] = "http://" + MDNS.hostname(i) + ".local:3000";
    Serial.println("url:" + url[i]);
    j++;
   }
  }

  ESPUI.updateLabel(URL1, String(url[0]));
  ESPUI.updateLabel(URL2, String(url[1]));
  ESPUI.updateLabel(URL3, String(url[2]));

  nameID = Project.c_str();
  version = FirmwareVer.c_str();
  ESPUI.updateLabel(nameLabel, String(nameID));
  ESPUI.updateLabel(firmwarelabel, String(version));

  // Start the Modbus TCP server:
  // Port number 502, maximum of 4 clients in parallel, 10 seconds timeout
  mb.start(502, 4, 120000);

}

void loop()
{
  unsigned long currentMillis = millis();
  static unsigned long httpmillis = 0;
  //Serial.print(".");

  if (WiFi.status() != WL_CONNECTED) {
      Serial.println("Wi-Fi disconnected! Attempting to reconnect...");
      WiFi.reconnect();
      if (WiFi.status() == WL_CONNECTED) {
        wifistat = "Connected to Wi-Fi!";
        Serial.println(wifistat);
        ESPUI.updateLabel(wifiLog, String(wifistat));
        Serial.printf("New IP Address: %s\n", WiFi.localIP().toString().c_str());
        ESPUI.updateLabel( myIP, String(WiFi.localIP().toString()));

        if(mdns_init()!= ESP_OK){
            Serial.println("mDNS failed to start");
            return;
          } else {
            Serial.println("mDNS Started");
          }

          int nrOfServices = MDNS.queryService("http", "tcp");
          
          if (nrOfServices == 0) {
              Serial.println("No services were found.");
          }
          int j = 0;
          
          for (int i = 0; i < nrOfServices; i=i+1) {
          // print service information
          Serial.print("Hostname ");
          Serial.println(i);
          Serial.println(MDNS.hostname(i));
          if (MDNS.hostname(i).indexOf("smartpeir") != -1){
            url[j] = "http://" + MDNS.hostname(i) + ".local:3000";
            Serial.println("url:" + url[i]);
            j++;
          }
          }

        recon = 0;
      } else {
        recon++;
        wifistat = "Failed to reconnect to Wi-Fi.";
        Serial.println(wifistat);
        ESPUI.updateLabel(wifiLog, String(wifistat));
        delayMicroseconds(1000000);
        if (recon == 60){
          ESP.restart();
        }
      }

  }

  if (currentMillis - httpmillis > 1000){
    httpmillis = currentMillis;
      if (WiFi.status() == WL_CONNECTED){
        HTTPClient http;
        for (int i = 0; i < 3; i++){
          // Serial.println("Check i= " + String(i));
          if (url[i].equals("")){
            break;
          }
          http.begin(url[i].c_str());

          // Send HTTP GET request
          int httpResponseCode = http.GET();
          
          if (httpResponseCode>0) {
            // Serial.print("HTTP Response code: ");
            // Serial.println(httpResponseCode);
            String payload = http.getString();
            // Serial.println(payload);
            ar[i] = payload.toInt();
          }
          else {
            Serial.print("Error code: ");
            Serial.println(httpResponseCode);
            ESPUI.updateLabel(httpbug, String(httpResponseCode));
          }
          // Free resources
          http.end();
        }
      }
  }

  mb.registerWorker(ClAddr, READ_HOLD_REGISTER, &FC03);

  if (currentMillis - readmillis > 5000){
    readmillis = currentMillis;
    avgHeight();
    calHeight();
    //calBuoyancy();
    EnableAlarm();
  }

  if (currentMillis - interupmillis > 60000){
    interupmillis = currentMillis;
    log();
    rssi = WiFi.RSSI();
    ESPUI.updateLabel(sig, String(rssi));
  }
   
}

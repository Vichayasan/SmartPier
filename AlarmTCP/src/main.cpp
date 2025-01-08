#include <WiFiManager.h>
#include <ArduinoModbus.h>
//#include <ArduinoRS485.h>
#include <SPI.h>
#include <ArduinoJson.h>
#include <EEPROM.h>
#include <ESPUI.h>
#include "http_ota.h"
#include <ESPmDNS.h>
#include <Adafruit_MCP23008.h>

WiFiManager wifiManager;
WiFiClient wifiClient;
WiFiClient client[3];
WiFiServer wifiServer(502);
ModbusTCPServer modbusTCPServer;
ModbusTCPClient modbusTCPClient(wifiClient);
IPAddress ip(192, 168, 0, 101);
Adafruit_MCP23008 mcp;

String host = "ServerDemo";
String deviceToken = "";
String ServAddress = "192.168.0.1";
String localIP = "";
String host2 = "";
int ClAddr;
// float value = 0;
int periodSendTelemetry = 30;
int TCPaddress = 3;
int periodOTA = 60;
float h1, h2, h3 = 0; 
float avgh;
int immersH, totalH, immersW;
int lenght, width;
int Density = 1000;
//float Gravity = 981;
int greenW, yellowW; //relay3,  relay2
int greenH, yellowH;

unsigned long previousMillis = 0;
unsigned long readmillis = 0;

float ar[3];

uint16_t nameLabel, idLabel, firmwarelabel, myIP;
uint16_t clientAddr, serverIP;
uint16_t interval, otaTime, hTotal, dens, grav, x, y;
uint16_t greenAW, yellowAW, greenAH, yellowAH;
uint16_t ul1, ul2, ul3;

void configModeCallback(WiFiManager *myWiFiManager);
void enterConnectionCallback(Control *sender, int type);
void enterSettingCallback(Control *sender, int type);
void eepromWrite();
void eepromRead();
float avgHeight();
void enterAlarmSetCallback(Control *sender, int type);

void _initWiFiManager()
{
  wifiManager.setAPCallback(configModeCallback);
  wifiManager.setConfigPortalTimeout(60); // auto close configportal after n seconds
  wifiManager.setAPClientCheck(true);      // avoid timeout if client connected to softap
  wifiManager.setBreakAfterConfig(true);   // always exit configportal even if wifi save fails
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
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
}

void _initModbusServ()
{
  if (!modbusTCPServer.begin(502)) {
    Serial.println("Failed to start Modbus TCP Server!");
    while (1);
  }
}

void _initModbusClient()
{
  while(!modbusTCPClient.begin(ip)) {
    Serial.println("Failed to connect Modbus TCP Server!");
  }
  Serial.println("Modbus TCP Client connected");
}

void modbusRegis( int IPadress, int NumBit)
{
  // Configure Modbus registers
  modbusTCPServer.configureHoldingRegisters(IPadress, NumBit); // 10 holding registers
  Serial.println("Modbus server ready.");
}

void reConnect2Client() 
{
  // Check for new client connections
  WiFiClient client = wifiServer.available();

  if (client) {
    //Serial.println("New client connected");
    
    modbusTCPServer.accept(client); // Accept the client connection

    // Optionally print the client's IP
    Serial.printf("Client IP: %s\n", client.remoteIP().toString().c_str());
  }

  // Process Modbus requests from clients
  modbusTCPServer.poll();
}

void reConnect2Serv() 
{
  // Check for new client connections
  //_initModbusServ();

  if (!modbusTCPClient.connected()) {
    Serial.println("Attempting to connect to Modbus TCP server");
    _initModbusClient();

  }
}

void servRequest(int address)
{
  // Simulate a value for holding register 0
  float value = modbusTCPServer.holdingRegisterRead(address);
  Serial.printf("Register %d value: %2f\n", address, value);
  //delayMicroseconds(1000); // Update register every second
  ar[address] = value;

}

float avgHeight()
{
  h1 = ar[1];
  h2 = ar[2];
  h3 = ar[3];
  Serial.printf("h1: %2f, h2: %2f, h3: %2f\n", h1, h2 ,h3);

  ESPUI.updateLabel(ul1, String(h1).c_str());
  ESPUI.updateLabel(ul2, String(h2).c_str());
  ESPUI.updateLabel(ul3, String(h3).c_str());

  avgh = (h1 + h2 + h3) / 3;
  Serial.printf("Average Height: %2f\n", avgh);
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
  
  if (immersW <= greenW || immersH <= greenH){
    mcp.digitalWrite(2, HIGH);
  }

  //yellow
  else if (immersW <= yellowW || immersH <= yellowH){
    mcp.digitalWrite(1, HIGH);
  }

  //red = relay1 pin0
  else{
    mcp.digitalWrite(0, HIGH);
    //mcp.digitalWrite(3, HIGH);
  }
}

void holdingWrite(int address, uint16_t value)
{
  // Simulate a value for holding register 0
  modbusTCPClient.holdingRegisterWrite(address, value);
  Serial.printf("Register %d value: %d\n", address, value);
  //delay(1000); // Update register every second
}

void _initUI(){
  host2 = "AlarmTCP" + deviceToken;
  //host2 = "SmartPeir";
  MDNS.begin(host2.c_str());
  //WiFi.mode(WIFI_MODE_AP);
  WiFi.softAPConfig(IPAddress(192, 168, 1, 1), IPAddress(192, 168, 1, 1), IPAddress(255, 255, 255, 0));
  WiFi.softAP(host2.c_str());
  //Finally, start up the UI.
  //This should only be called once we are connected to WiFi.
  
}

void setUpUI() {
  Serial.println("Start UI");

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
  idLabel = ESPUI.addControl(Label, "Device ID", String(deviceToken), Peterriver, maintab);
  firmwarelabel = ESPUI.addControl(Label, "Firmware Version", String(FirmwareVer), Peterriver, maintab);
  myIP = ESPUI.addControl(Label, "IP Address", String(localIP), Peterriver, maintab);

  auto network = ESPUI.addControl(Tab, "", "Connection");
  serverIP = ESPUI.addControl(Text, "Server IP Adress", String(ServAddress), Peterriver, network, enterConnectionCallback);
  clientAddr = ESPUI.addControl(Number, "Client Address", String(ClAddr), Peterriver, network, enterConnectionCallback);
  ESPUI.addControl(Button, "Save", "SAVE", Dark, network, enterConnectionCallback);

  auto alarmset = ESPUI.addControl(Tab, "", "Alarm");
  ESPUI.addControl(Separator, "Height Configuration","", None, alarmset);
  greenAH = ESPUI.addControl(Number, "Maximum of Red Level(cm)", String(greenH), Peterriver, alarmset, enterAlarmSetCallback);
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
  ul1 = ESPUI.addControl(Label, "Ultrasonic Device 1", String(h1), Peterriver, log);
  ul2 = ESPUI.addControl(Label, "Ultrasonic Device 2", String(h2), Peterriver, log);
  ul3 = ESPUI.addControl(Label, "Ultrasonic Device 3", String(h3), Peterriver, log);

  Serial.println("End UI");
  String hostUI = "AlarmTCP" + deviceToken;
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

void eepromRead(){
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

  // for (int len = 0; len < 20; len++){
  //   char data1 = EEPROM.read(addr + len);
  //   Serial.println("data1: " +  String(EEPROM.read(addr + len)));
  //   if (data1 == '\0' || data1 == 255 || data1 == 20 || data1 == 0)
  //     break;
  //   ServAddress += data1;
  // }
  // addr += sizeof(ServAddress);

  // for (int len = 0; len < 50; len++){
  //     char data3 = EEPROM.read(addr + len);
  //     Serial.println("data3: " +  String(EEPROM.read(addr + len)));
  //     if(data3 == '\0' || data3 == 255 || data3 == 20 || data3 == 0) break;
  //     deviceToken += data3;
  //   }
  
  EEPROM.end();

  if (periodSendTelemetry == 255 || periodSendTelemetry == -1){
    periodOTA = 30;
    periodSendTelemetry = 60;
  }

  // ip.fromString(ServAddress);

  // Print to Serial
  Serial.println("----- Read EEPROM Storage value by ESPUI -----");
  Serial.println("Server Address: "+ String(ServAddress));
  Serial.printf("periodSendTelemetry: %d\n", periodSendTelemetry);
  Serial.printf("periodOTA: %d\n", periodOTA);
  Serial.printf("Total Height: %d\n", totalH);
  Serial.printf("lenght: %d\n", lenght);
  Serial.printf("width: %d\n", width);
  Serial,printf("DeviceID: %d\n", deviceToken);

  // ESPUI.updateText(serverIP, String(ServAddress));
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
  ESPUI.updateLabel(idLabel, String(deviceToken));
}

void _initMCP()
{
  mcp.begin();      // use default address 0
  mcp.pinMode(0, OUTPUT);
  mcp.pinMode(1, OUTPUT);
  mcp.pinMode(2, OUTPUT);
  mcp.pinMode(3, OUTPUT);
}

void setup(){

  Serial.begin(115200);
  Project = "SmartPier";
  FirmwareVer = 0.0;

  //wifiManager.resetSettings();

  Serial.println("WiFi is connecting...");
  _initWiFiManager();
  Serial.println("WiFi is connected!");

  printWifiStatus();

  

  getMac();

  delayMicroseconds(300000);
  _initUI();
  setUpUI();
  delayMicroseconds(300000);

  eepromRead();

  wifiServer.begin();

  Serial.println("Start Modbus Server...");
  _initModbusServ();

  modbusRegis(0, 30);
  

  Serial.println("Start Modbus Client...");
  _initModbusClient();
  delayMicroseconds(1000);

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

}

void loop()
{
  //unsigned long currentMillis = millis();

  Serial.print(".");
  WiFiClient newClient = wifiServer.available();
    if (newClient) {
    // a new client connected
      Serial.println("new client");

      Serial.println("debug 1");
      // let the Modbus TCP accept the connection 
      
      Serial.println("debug 2");
      for (int i = 0; i < 3; i++){
        modbusTCPServer.accept(newClient);
        //if (client[i].connected()) {
        client[i] = newClient;
        Serial.printf("Client added to slot %d. IP: %s\n", i, newClient.remoteIP().toString().c_str());
        //delayMicroseconds(1000000);
        //break;
        //}

      }

      for (int i = 0; i < 3; i++){
        modbusTCPServer.accept(client[i]);
          if (client[i].connected()){
            // poll for Modbus TCP requests, while client connected
            Serial.println("debug 3");
            modbusTCPServer.poll();
            Serial.println("debug 4");
            // update the LED
            for (int i = 0; i <= TCPaddress; i++){
              servRequest(i);
              delayMicroseconds(50000);
            }
            Serial.printf("\n");
          //value++;
        }
    }
    }
    static unsigned long readmillis =0;    
  if (millis() - readmillis > 10000){
    readmillis = millis();
    avgHeight();
    calHeight();
    //calBuoyancy();
    EnableAlarm();
  }
  // delay(1000);

  // if (!modbusTCPClient.connected()) {
  //   Serial.println("Attempting to connect to Modbus TCP server");
  //   _initModbusClient();

  // }

  // if(currentMillis - previousMillis >= periodSendTelemetry * 1000){
  //   previousMillis = currentMillis;
  //   reConnect2Serv();
  //   holdingWrite(ClAddr, immersW);
  // }
   
}
#include <WiFiManager.h>
#include <ArduinoModbus.h>
//#include <ArduinoRS485.h>
#include <SPI.h>
#include <ArduinoJson.h>
#include <ESPAsyncWebServer.h>
#include <ESPmDNS.h>
#include <ESPUI.h>
#include <EEPROM.h>

WiFiManager wifiManager;
WiFiClient wifiClient;
ModbusTCPClient modbusTCPClient(wifiClient);
IPAddress ip(192, 168, 137, 185);

String host = "ClientDemo";
//static int value = 0;
String deviceToken = "";
String ServAddress = "";
String localIP = "";
String host2 = "";
int ClAddr;
int periodSendTelemetry = 120;
int value = 0;

uint16_t nameLabel, idLabel, firmwarelabel, myIP;
uint16_t clientAddr, serverIP;
uint16_t interval;

void configModeCallback(WiFiManager *myWiFiManager);
void enterConnectionCallback(Control *sender, int type);
void enterSettingCallback(Control *sender, int type);
void eepromWrite();
void eepromRead();

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

void printWifiStatus() {
  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print your WiFi shield's IP address:
  IPAddress ip = WiFi.localIP();
  localIP = ip.toString();
  Serial.print("IP Address: ");
  Serial.println(ip);

  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
}

void _initModbus()
{
  while(!modbusTCPClient.begin(ip)) {
    Serial.println("Failed to connect Modbus TCP Server!");
    delay(1000);
  }
  Serial.println("Modbus TCP Client connected");
}

void reConnection() {
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

void _initUI(){
  host2 = "SmartPeir:" + deviceToken;
  //host2 = "SmartPeir";
  MDNS.begin(host2.c_str());
  WiFi.softAPConfig(IPAddress(192, 168, 1, 1), IPAddress(192, 168, 1, 1), IPAddress(255, 255, 255, 0));
  WiFi.softAP(host2.c_str());
  //Finally, start up the UI.
  //This should only be called once we are connected to WiFi.
  ESPUI.begin(host2.c_str());
}

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
  nameLabel = ESPUI.addControl(Label, "Device Name", "AIRMASS 2.5 VERSION 3.5", Peterriver, maintab);
  idLabel = ESPUI.addControl(Label, "Device ID", String(deviceToken), Peterriver, maintab);
  firmwarelabel = ESPUI.addControl(Label, "Firmware", "0.0.0", Peterriver, maintab);
  myIP = ESPUI.addControl(Label, "IP Address", String(localIP), Peterriver, maintab);

  auto network = ESPUI.addControl(Tab, "", "Connection");
  serverIP = ESPUI.addControl(Text, "Server IP Adress", String(ServAddress), Peterriver, network, enterConnectionCallback);
  clientAddr = ESPUI.addControl(Number, "Client Address", String(ClAddr), Peterriver, network, enterConnectionCallback);
  ESPUI.addControl(Button, "Save", "SAVE", Dark, network, enterConnectionCallback);

  auto setting = ESPUI.addControl(Tab, "", "Setting");
  interval = ESPUI.addControl(Number, "Interval Time", String(periodSendTelemetry), Peterriver, setting, enterSettingCallback);
  ESPUI.addControl(Button, "Save", "SAVE", Dark, setting, enterSettingCallback);

}

void enterConnectionCallback(Control *sender, int type) {
  Serial.println(sender->value);
  ESPUI.updateControl(sender);
  if(type == B_UP) {
    Serial.println("Saving Offset to EPROM...");
    Control* address_ = ESPUI.getControl(serverIP);
    Control* Cl_ = ESPUI.getControl(clientAddr);
    ClAddr = Cl_->value.toInt();
    ServAddress = address_->value.c_str();
    ip.fromString(ServAddress);
    eepromWrite();
  }
}

void enterSettingCallback(Control *sender, int type){
  Serial.println(sender->value);
  ESPUI.updateControl(sender);
  if(type == B_UP) {
    Serial.println("Saving Offset to EPROM...");
    
    eepromWrite();
  }
}

void eepromWrite(){

  char data1[40];
  char data2[40];
  char data3[40];

  ServAddress.toCharArray(data1, 20);
  EEPROM.begin(180);
  int addr = 0;
  for (int len = 0; len < ServAddress.length(); len++) {
      EEPROM.write(addr + len, data1[len]);  // Write each character
    }
    EEPROM.write(addr + ServAddress.length(), '\0');
  
  EEPROM.put(addr, ClAddr);
  addr += sizeof(ClAddr);

  EEPROM.end();
}

void eepromRead(){
  Serial.println("Reading credentials from EEPROM...");
  EEPROM.begin(180); // Ensure enough size for data
  int addr = 0;
  for (int len = 0; len < 20; len++){
    char data1 = EEPROM.read(addr + len);
    if (data1 == '\0' || data1 == 255)
      break;
    ServAddress += data1;
  }
  EEPROM.get(addr, ClAddr);
  addr += sizeof(ClAddr);

  EEPROM.end();
  ip.fromString(ServAddress);

  // Print to Serial
  Serial.println("----- Read EEPROM Storage value by ESPUI -----");
  Serial.println("Server Address: "+ String(ServAddress));

  ESPUI.updateText(serverIP, String(ServAddress));
  ESPUI.updateNumber(clientAddr, ClAddr);

}

void setup(){

  Serial.begin(115200);

  Serial.println("WiFi is connecting...");
  _initWiFiManager();
  Serial.println("WiFi is connected!");

  printWifiStatus();

  getMac();

  eepromRead();

  delayMicroseconds(1000);
  _initUI();
  setUpUI();
  delayMicroseconds(1000);

  Serial.println("Start Modbus...");
  _initModbus();

}

void loop()
{
  const unsigned long currentMillis = millis();
  const unsigned long time2send = periodSendTelemetry * 1000;
  if (currentMillis % time2send == 0){
    reConnection();
    delay(1000);
    holdingWrite(ClAddr, value);
    value++;
  }
  
}
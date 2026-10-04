#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ESP32Time.h>
#include <classes.h>
#include <nimBLEDevice.h>
#include <vector>
using namespace std;

#define SERVICE_UUID           "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_RX "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_TX "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"

bool NOTIFICATION = false;
volatile bool NEW_NOTIFICATION = false;
volatile bool NEW_TIME = false;
string NOTIFICATION_RAW_DATA;
int TIME_SYNC[6] = {0,0,0,0,0,0};
vector<notif> currNotifs;
bool MILLITARY_TIME = false;
bool isPM = false; 

Adafruit_SSD1306 display(128, 64, &Wire, -1);
ESP32Time rtc;
ServerCallbacks serverCallbacks;
CharacteristicCallbacks chrCallbacks;
NimBLECharacteristic* pTxCharacteristic = nullptr;

const uint8_t PROGMEM notifBell[] = {
    0x01, 0x80,  //        **        
    0x03, 0xC0,  //       ****       
    0x07, 0xE0,  //      ******      
    0x07, 0xE0,  //      ******      
    0x07, 0xE0,  //      ******      
    0x0F, 0xF0,  //     ********     
    0x0F, 0xF0,  //     ********     
    0x0F, 0xF0,  //     ********     
    0x1F, 0xF8,  //    **********    
    0x1F, 0xF8,  //    **********    
    0x3F, 0xFC,  //   ************   
    0x7F, 0xFE,  //  ==============  (bell rim)
    0xFF, 0xFF,  //  ==============  
    0x00, 0x00,  //                  
    0x03, 0xC0,  //       ****       (clapper)
    0x01, 0x80   //        **        
};

void createNotif(){
  int firstPos = NOTIFICATION_RAW_DATA.find('|');
  int secondPos = NOTIFICATION_RAW_DATA.rfind('|');
  int endPos = NOTIFICATION_RAW_DATA.find('\0');
  string app = NOTIFICATION_RAW_DATA.substr(0, firstPos);
  string header = NOTIFICATION_RAW_DATA.substr(firstPos + 1, secondPos - firstPos - 1); 
  string body = NOTIFICATION_RAW_DATA.substr(secondPos + 1, endPos - secondPos);
  currNotifs.emplace_back(app, header, body);
}

void vibrateMotor(){
  for(int i = 0; i < 3; i++){
    digitalWrite(D3,HIGH);
    delay(500);
    digitalWrite(D3,LOW);
    delay(250);
  }
}

void startBLE(){
  NimBLEDevice::init("BR-Smart-Pager");
  NimBLEDevice::setPower(9);
  NimBLEServer* pServer = NimBLEDevice::createServer();
  pServer->advertiseOnDisconnect(true);
  pServer->setCallbacks(&serverCallbacks);
  NimBLEAdvertising* pAdvertising = pServer->getAdvertising();
  NimBLEAdvertisementData advData{};
  advData.setFlags(0x06);
  advData.setName("BR-Smart-Pager");
  advData.addServiceUUID(SERVICE_UUID);
  pAdvertising->setAdvertisementData(advData);
  NimBLEService* pService = pServer->createService(SERVICE_UUID);
  NimBLECharacteristic* pRxCharacteristic = pService->createCharacteristic(
  CHARACTERISTIC_UUID_RX,NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_NR);
  pRxCharacteristic->setCallbacks(&chrCallbacks);
  pTxCharacteristic = pService->createCharacteristic(CHARACTERISTIC_UUID_TX,NIMBLE_PROPERTY::NOTIFY);

  pAdvertising->start();

  Serial.println("BLE Started. Advertising as 'BR-Smart-Pager'...");
}

void displayNotifIcon(){
  display.drawBitmap(0, 0, notifBell, 16, 16, WHITE);
}

void displayHomeScreen(){
  display.drawFastHLine(0, 64/4, 128, SSD1306_WHITE);
  display.drawFastHLine(0,64/4*3, 128, SSD1306_WHITE);
  display.setTextSize(3);
  String time = rtc.getTime();
  String stHr = time.substring(0,2);
  int hr = stHr.toInt();
  if(!MILLITARY_TIME && hr >=13){
    hr = hr - 12;
    stHr = String(hr);
    if(stHr.length() < 2){
      stHr = '0' + stHr;
    }
    time[0] = stHr[0];
    time[1] = stHr[1];
    isPM = true;
  }else if(hr == 0 && !MILLITARY_TIME){
    hr = 12;
    stHr = String(hr);
    time[0] = stHr[0];
    time[1] = stHr[1];
    isPM = true;
    isPM = false;
  }else{
    isPM = false;
  }
  if(time[0] == '0'){
    time = time.substring(1,5);
  } else{
    time = time.substring(0,5);
  }
  int16_t x1,y1;
  uint16_t w,h;
  display.getTextBounds(time,0,0,&x1,&y1,&w,&h);
  display.setCursor((128-w)/2,(64-h)/2);
  display.print(time);
  display.setTextSize(1);
  String date = rtc.getDate();
  String day = date.substring(0, 3);
  String month = date.substring(5, 8);
  String dayNum = date.substring(9, 11);
  display.setCursor(128/8 * 7 -5,64/2 -6);
  display.print(day);
  display.setCursor(128/8 * 7 -5,64/2 +2);
  if(isPM){
    display.print("PM");
  }else{
    display.print("AM");
  }
  display.setCursor(0,64/2 -6);
  display.println(month);
  display.println(dayNum);
  if(NOTIFICATION){
    displayNotifIcon();
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println("Hello World");
  startBLE();
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  //rtc.setTime(0, 34, 12, 30, 9, 2026);
  pinMode(D0,INPUT_PULLUP);
  pinMode(D1,INPUT_PULLUP);
  pinMode(D2,INPUT_PULLUP);
  pinMode(D3,OUTPUT);

}

void loop() {
  displayHomeScreen();
  if(NEW_TIME){
    rtc.setTime(TIME_SYNC[0], TIME_SYNC[1], TIME_SYNC[2], TIME_SYNC[3], TIME_SYNC[4], TIME_SYNC[5]);
    NEW_TIME = false;
  }
  if(NEW_NOTIFICATION){
    displayNotifIcon();
    vibrateMotor();
    createNotif();
    NOTIFICATION = true;
    NEW_NOTIFICATION = false;
    string app = currNotifs[currNotifs.size() -1].getApp();
    string header = currNotifs[currNotifs.size() -1].getHeader();
    string body = currNotifs[currNotifs.size() -1].getBody();
    display.clearDisplay();
    display.setCursor(0,0);
    display.setTextSize(1);
    display.println(String(app.c_str()));
    display.setTextSize(2);
    display.println(String(header.c_str()));
    display.setTextSize(1);
    display.println(String(body.c_str()));
    display.display();
    delay(10000);
  }
  display.display();
  display.clearDisplay();
  if(digitalRead(D0) == LOW){
    Serial.println("Button Pressed!");
    vibrateMotor();
    NOTIFICATION = false;
  }
}
/*
ESPboy Meteo station by RomanS

ESPboy project page:
https://hackaday.io/project/164830-espboy-beyond-the-games-platform-with-wifi
*/

#include <Wire.h>
#include <Adafruit_NeoPixel.h>
#include <Adafruit_MCP23017.h>
#include <Adafruit_MCP4725.h>wea
#include <TFT_eSPI.h>
#include "ESPboyOTA.h"

#include <ESP8266WiFi.h>
#include "ESPboyLogo.h"
#include <BMx280I2C.h>
#include "RTClib.h"
#include "Adafruit_SGP30.h"
 
#define LEDquantity     1
#define MCP23017address 0 // actually it's 0x20 but in <Adafruit_MCP23017.h> lib there is (x|0x20) :)

//buttons
#define PAD_LEFT        0x01
#define PAD_UP          0x02
#define PAD_DOWN        0x04
#define PAD_RIGHT       0x08
#define PAD_ACT         0x10
#define PAD_ESC         0x20
#define PAD_LFT         0x40
#define PAD_RGT         0x80
#define PAD_ANY         0xff
 
#define csTFTMCP23017pin  8 //chip select pin on the MCP23017 for TFT display 
#define LEDpin            D4
#define SOUNDpin          D3
 
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(LEDquantity, LEDpin, NEO_GRB + NEO_KHZ800);

Adafruit_MCP23017 mcp;
Adafruit_MCP4725 dac;
BMx280I2C bmx280(0x76);
RTC_DS3231 rtc;
Adafruit_SGP30 sgp;
TFT_eSPI tft = TFT_eSPI();
ESPboyOTA* OTAobj = NULL;
 
const char *daysOfTheWeek[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
const char *months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sen", "Oct", "Nov", "Dec"};
static DateTime now;
static uint16_t lcdbrightness = 650;
static uint16_t buttonspressed;
static float hum;  //relative humidity [%]
static float ahum; //absolute humidity [%]
static float temp; //temperature [C]
static double pres; //atm.pressure [Pa]



uint8_t getKeys() { return (~mcp.readGPIOAB() & 255); }


void runButtonsCommand(uint8_t bt){
  pixels.setPixelColor(0, pixels.Color(0,0,20));
  pixels.show();
  tone (SOUNDpin, 800, 20);
  if (bt&PAD_LFT && lcdbrightness > 300) 
  {
    lcdbrightness-=30;
    dac.setVoltage(lcdbrightness, false);
  }
  if (bt&PAD_RGT && lcdbrightness < 650){ 
    lcdbrightness+=30;
    dac.setVoltage(lcdbrightness, false);
  }
  if (bt&PAD_LEFT) {
      if (now.day() < 31) rtc.adjust (DateTime(now.year(), now.month(), (now.day()+1), now.hour(), now.minute(), now.second()));
      else rtc.adjust (DateTime(now.year(), now.month(), 1, now.hour(), now.minute(), now.second()));
      now = rtc.now();
  }
  if (bt&PAD_RIGHT) {
       if (now.month() < 12) rtc.adjust (DateTime(now.year(), now.month()+1, now.day(), now.hour(), now.minute(), now.second()));
       else rtc.adjust (DateTime(now.year(), 1, now.day(), now.hour(), now.minute(), now.second()));
       now = rtc.now();
  }
  if (bt&PAD_UP){
       rtc.adjust (DateTime(now.year()+1, now.month(), now.day(), now.hour(), now.minute(), now.second()));
       now = rtc.now();
  }
  if (bt&PAD_DOWN){
       rtc.adjust (DateTime(now.year()-1, now.month(), now.day(), now.hour(), now.minute(), now.second()));
       now = rtc.now();
  }
  if (bt&PAD_ACT){
       if (now.hour() < 24) rtc.adjust (DateTime(now.year(), now.month(), now.day(), now.hour()+1, now.minute(), now.second()));
       else rtc.adjust (DateTime(now.year(), now.month(), now.day(), 0, now.minute(), now.second()));
       now = rtc.now();
  }
  if (bt&PAD_ESC){
       if (now.minute() < 60) rtc.adjust (DateTime(now.year(), now.month(), now.day(), now.hour(), now.minute()+1, 0));
       else rtc.adjust (DateTime(now.year(), now.month(), now.day(), now.hour(), 0, 0));
       now = rtc.now();
   }
}
  

void batVoltageDraw(){
  uint16_t volt;
  volt = analogRead(A0);
  volt = map(volt, 820, 1024, 0, 99);
  tft.setTextSize(1);
  tft.setTextColor(TFT_WHITE);
  tft.setCursor(110, 120);
  tft.print(volt);
  tft.print("%");
}


void drawled(){
  int8_t temp = bmx280.getTemperature();
  pixels.setPixelColor(0, pixels.Color(0,10,0));
  if (temp < 0) pixels.setPixelColor(0, pixels.Color(0,0,10));
  if (temp > 35) pixels.setPixelColor(0, pixels.Color(10,0,0));
 // if (sgp.eCO2 > 1200 || sgp.TVOC > 2000) pixels.setPixelColor(0, pixels.Color(10,0,0));
  pixels.show();
}

 
/*
void printserial(){
  Serial.print("T:");
  Serial.print((int)bmx280.getTemperature());
  Serial.print("C ");
  Serial.print("H:");
  Serial.print((int)bmx280.getHumidity());
  Serial.print("% ");
  Serial.print("P:");
  Serial.print((int)((float)bmx280.getPressure() / 133.3));
  Serial.print("mmH  ");
 
  Serial.print(now.hour(), DEC);
  Serial.print(":");
  Serial.print(now.minute(), DEC);
  Serial.print(".");
  Serial.print(now.second(), DEC);
  Serial.print("  ");
  Serial.print(now.day(), DEC);
  Serial.print(" ");
  Serial.print(months[now.month() - 1]);
  Serial.print(" ");
  Serial.print(now.year(), DEC);
  Serial.print(" ");
  Serial.print(daysOfTheWeek[now.dayOfTheWeek()]);
  Serial.println();
  Serial.println();
}
*/
 
 
void printtft(){
 
//draw date
  tft.setTextColor(TFT_YELLOW);
  tft.setTextSize(1);
  tft.setCursor (20, 0);
  if (now.day() < 10) tft.print ("0");
  tft.print (now.day());
  tft.print (" ");
  tft.print (months[now.month() - 1]);
  tft.print (" ");
  tft.print (now.year());
  tft.print (" ");
  tft.print (daysOfTheWeek[now.dayOfTheWeek()]);
 
//draw time
  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(3);
  tft.setCursor (21, 18);
  if (now.hour() < 10) tft.print ("0");
  tft.print (now.hour());
  tft.print (":");
  if (now.minute() < 10) tft.print ("0");
  tft.print (now.minute());
 
//draw temp
  tft.setTextColor(TFT_GREEN);
  tft.setCursor (0, 49);
  tft.setTextSize(1);
  tft.print ("Temp   ");
  tft.setTextSize(2);
  tft.print ((int16_t)round(temp));
  tft.setTextSize(1);
  tft.print ("C");
 
//draw humidity
  tft.setTextColor(TFT_GREEN);
  tft.setCursor (0, 69);
  tft.setTextSize(1);
  tft.print ("Humid  ");
  tft.setTextSize(2);
  tft.print ((int16_t)round(hum));
  tft.setTextSize(1);
  tft.print ("% ");
  tft.setTextSize(2);
  tft.print ((int16_t)round(ahum));
  tft.setTextSize(1);
  tft.print ("g/m3");
 
//draw pressure
  tft.setTextColor(TFT_GREEN);
  tft.setCursor (0, 89);
  tft.setTextSize(1);
  tft.print ("Atm.pr ");
  tft.setTextSize(2);
  tft.print ((int16_t)round(pres));
  tft.setTextSize(1);
  tft.print ("mmHg");

  //draw CO2
 /*
  tft.setTextColor(TFT_MAGENTA);
  tft.setCursor (0, 109);   
  tft.setTextSize(1);
  tft.print ("eCO2:  ");
  tft.print (sgp.eCO2);
  tft.print (" ppm");
  tft.setCursor (0, 119);
  tft.print ("TVOC:  ");
  tft.print (sgp.TVOC);
  tft.print (" ppb");
*/
  
  batVoltageDraw();
}
 
 
void setup() {
  Serial.begin(9600); //serial init
  Wire.begin(); //I2C init
  delay(100);
  WiFi.mode(WIFI_OFF); // to safe some battery power
 
//LED init
  pinMode(LEDpin, OUTPUT);
  pixels.begin();
  delay(100);  
  pixels.setPixelColor(0, pixels.Color(0,0,0));
  pixels.show();

//MCP23017 init
  mcp.begin(MCP23017address);
  delay(100);
  for (int i = 0; i < 8; ++i) {
    mcp.pinMode(i, INPUT);
    mcp.pullUp(i, HIGH);}
  
 
//TFT init    
  mcp.pinMode(csTFTMCP23017pin, OUTPUT);
  mcp.digitalWrite(csTFTMCP23017pin, LOW);
  tft.begin();
  delay(100);
  tft.setRotation(0);
  tft.fillScreen(TFT_BLACK);
 
//draw ESPboylogo 
  tft.drawXBitmap(30, 24, ESPboyLogo, 68, 64, TFT_YELLOW);
  tft.setTextSize(1);
  tft.setTextColor(TFT_YELLOW);
  tft.setCursor(44,102);
  tft.print ("Weather");
 
//sound init and test
  pinMode(SOUNDpin, OUTPUT);
  tone(SOUNDpin, 200, 100);
  delay(100);
  tone(SOUNDpin, 100, 100);
  delay(100);
  noTone(SOUNDpin);

  if (getKeys()&PAD_ACT || getKeys()&PAD_ESC) OTAobj = new ESPboyOTA(&tft, &mcp);

 
//BME280 init
  tft.setTextColor(TFT_RED);
  tft.setTextSize(1);
  tft.setCursor(0,120);
  if (!bmx280.begin()){
    Serial.println("BMP280 FAILED");
    tft.print ("Weather module FAILED");
    while (1) delay(100);}
  bmx280.resetToDefaults();
  bmx280.writeOversamplingPressure(BMx280MI::OSRS_P_x16);
  bmx280.writeOversamplingTemperature(BMx280MI::OSRS_T_x16);
  bmx280.writeOversamplingHumidity(BMx280MI::OSRS_H_x16);
  bmx280.measure();
  while (!bmx280.hasValue()) delay(100);
 
//RTC init
  if (!rtc.begin()) {
    tft.print ("Weather plugin FAILED");
    while (1) delay(100);}

//SGP30 init

//  if (!sgp.begin()){
//    tft.print ("Meteo plugin FAILED S");
//    while (1) delay(100);}

//DAC init 
dac.begin(0x60);
delay (100);
dac.setVoltage(lcdbrightness, false);

//clear TFT
  delay(2000);
  tft.fillScreen(TFT_BLACK);
}
 
 
void loop() {
 static long count; 
 if (millis() > count + 3000){
     count = millis();
     while (!bmx280.hasValue()) delay(100);
     now = rtc.now();
     bmx280.measure();
     temp = bmx280.getTemperature();
     hum = bmx280.getHumidity();
     if (hum >99) hum=99;
     ahum = 216.7f * ((hum / 100.0f) * 6.112f * exp((17.62f * temp) / (243.12f + temp)) / (273.15f + temp));
     if (ahum >99) ahum=99;
     pres = bmx280.getPressure() / 133.3d;
   //  sgp.setHumidity((double)ahum * 1000.0f);
   //  sgp.IAQmeasure();
     tft.fillScreen(TFT_BLACK);
     printtft();
     drawled();
 }
  uint8_t bt = getKeys();
  if (bt) { 
    runButtonsCommand(bt); 
    tft.fillScreen(TFT_BLACK); 
    printtft();
    drawled();
    delay(200);
  }
}

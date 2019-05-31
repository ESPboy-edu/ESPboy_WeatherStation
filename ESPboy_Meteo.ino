#include <Wire.h>
#include <Adafruit_NeoPixel.h>
#include <Adafruit_MCP23017.h>
#include <Adafruit_ST7735.h>
#include <Adafruit_GFX.h>
#include <ESP8266WiFi.h>
#include "ESPboyLogo.h"
#include <BMx280MI.h>
#include "RTClib.h"
 
#define LEDquantity     1
#define MCP23017address 0 // actually it's 0x20 but in <Adafruit_MCP23017.h> lib there is (x|0x20) :)

//buttons
#define UP_BUTTON       1
#define DOWN_BUTTON     2
#define LEFT_BUTTON     0
#define RIGHT_BUTTON    3
#define ACT_BUTTON      4
#define ESC_BUTTON      5
 
//SPI for LCD
#define csTFTMCP23017pin  8 //chip select pin on the MCP23017 for TFT display
#define TFT_RST          -1
#define TFT_DC            D8
#define TFT_CS           -1
 
#define LEDpin            D4
#define SOUNDpin          D3
 
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(LEDquantity, LEDpin, NEO_GRB + NEO_KHZ800);
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);
Adafruit_MCP23017 mcp;
BMx280I2C bmx280(0x76);
RTC_DS3231 rtc;
 
//const char *daysOfTheWeek[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
//const char *months[] = {"Jun", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sen", "Oct", "Nov", "Dec"};
static DateTime now;
uint8_t buttonspressed[8];


void drawled(){
  int8_t temp = bmx280.getTemperature();
  pixels.setPixelColor(0, pixels.Color(0,10,0));
  if (temp < 0) pixels.setPixelColor(0, pixels.Color(0,0,10));
  if (temp > 30) pixels.setPixelColor(0, pixels.Color(10,0,0));
  pixels.show();
}

 
uint8_t checkbuttons(){
  uint8_t check = 0;
  for (int i = 0; i < 8; i++){
    if(!mcp.digitalRead(i)) {
       buttonspressed[i] = 1;
       check++;
       delay(10);} // to avoid i2c bus ovelflow during long button keeping pressed
    else buttonspressed[i] = 0;
  }
  return (check);
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
 
  static float hum;  //relative humidity [%]
  static float ahum; //absolute humidity [%]
  static float temp; //temperature [C]
  static double pres; //atm.pressure [Pa]
 
//draw date
  tft.setTextColor(ST77XX_YELLOW);
  tft.setTextSize(2);
  tft.setCursor (6, 0);
  if (now.day() < 10) tft.print ("0");
  tft.print (now.day());
  tft.print (".");
  if (now.month() < 10) tft.print ("0");
  tft.print (now.month());
  tft.print (".");
  tft.print (now.year());
 
//draw time
  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(3);
  tft.setCursor (22, 32);
  if (now.hour() < 10) tft.print ("0");
  tft.print (now.hour());
  tft.print (":");
  if (now.minute() < 10) tft.print ("0");
  tft.print (now.minute());
 
//draw temp
  temp = bmx280.getTemperature();
  tft.setTextColor(ST77XX_GREEN);
  tft.setCursor (0, 70);
  tft.setTextSize(1);
  tft.print ("Tempr   ");
  tft.setTextSize(2);
  tft.print (round(temp));
  tft.setTextSize(1);
  tft.print (" C");
 
//draw humidity
  hum = bmx280.getHumidity();
  if (hum >99) hum=99;
  ahum = 216.7f * ((hum / 100.0f) * 6.112f * exp((17.62f * temp) / (243.12f + temp)) / (273.15f + temp));
  if (ahum >99) ahum=99;
  tft.setTextColor(ST77XX_GREEN);
  tft.setCursor (0, 90);
  tft.setTextSize(1);
  tft.print ("Humid   ");
  tft.setTextSize(2);
  tft.print (round(hum));
  tft.print ("/");
  tft.print (round(ahum));
  tft.setTextSize(1);
  tft.print (" %");
 
//draw pressure
  pres = bmx280.getPressure() / 133.3d;
  tft.setTextColor(ST77XX_GREEN);
  tft.setCursor (0, 110);
  tft.setTextSize(1);
  tft.print ("Press   ");
  tft.setTextSize(2);
  tft.print (round(pres));
  tft.setTextSize(1);
  tft.print (" mmHg");
}
 
 
void setup() {
  Serial.begin(115200); //serial init
  Wire.begin(); //I2C init
  delay(100);
  WiFi.mode(WIFI_OFF); // to safe some battery power
 
//LED init
  pinMode(LEDpin, OUTPUT);
  pixels.begin();
  delay(100);  
  pixels.setPixelColor(0, pixels.Color(0,0,0));
  pixels.show();

//TFT init    
  mcp.pinMode(csTFTMCP23017pin, OUTPUT);
  mcp.digitalWrite(csTFTMCP23017pin, LOW);
  tft.initR(INITR_144GREENTAB);
  delay (100);
  tft.setRotation(0);
  tft.fillScreen(ST77XX_BLACK);
 
//draw ESPboylogo 
  tft.drawXBitmap(30, 24, ESPboyLogo, 68, 64, ST77XX_YELLOW);
  tft.setTextSize(1);
  tft.setTextColor(ST77XX_YELLOW);
  tft.setCursor(50,102);
  tft.print ("Meteo");
 
//sound init and test
  pinMode(SOUNDpin, OUTPUT);
  tone(SOUNDpin, 200, 100);
  delay(100);
  tone(SOUNDpin, 100, 100);
  delay(100);
  noTone(SOUNDpin);
 
//buttons on mcp23017 init
  mcp.begin(MCP23017address);
  delay(100);
  for (int i=0;i<8;i++){ 
     mcp.pinMode(i, INPUT);
     mcp.pullUp(i, HIGH);}
 
//BME280 init
  tft.setTextColor(ST77XX_RED);
  tft.setTextSize(2);
  tft.setCursor(0,0);
  if (!bmx280.begin()){
    Serial.println("BMP280 FAILED");
    tft.print (" Meteo plugin FAILED");
    while (1) delay(100);}
  bmx280.resetToDefaults();
  bmx280.writeOversamplingPressure(BMx280MI::OSRS_P_x16);
  bmx280.writeOversamplingTemperature(BMx280MI::OSRS_T_x16);
  bmx280.writeOversamplingHumidity(BMx280MI::OSRS_H_x16);
  bmx280.measure();
  while (!bmx280.hasValue()) delay(100);
 
//RTC init
  if (!rtc.begin()) {
    Serial.println("RTC FAILED");
    tft.print (" Meteo plugin FAILED");
    while (1) delay(100);}

//clear TFT
  delay(2000);
  tft.fillScreen(ST77XX_BLACK);
}
 
 
void loop() {
  bmx280.measure();
  while (!bmx280.hasValue()) delay(100);
  now = rtc.now();
  tft.fillScreen(ST77XX_BLACK);
  //printserial();
  printtft();
  drawled();
  delay(5000);
}

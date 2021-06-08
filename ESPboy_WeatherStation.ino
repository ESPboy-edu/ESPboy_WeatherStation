/*
ESPboy Weather station by RomanS

ESPboy project page:
www.ESPboy.com
https://hackaday.io/project/164830-espboy-beyond-the-games-platform-with-wifi
*/


#include "lib/ESPboyInit.h"
#include "lib/ESPboyInit.cpp"
#include <BMx280I2C.h>
#include "RTClib.h"

ESPboyInit myESPboy;
BMx280I2C bmx280(0x76);
RTC_DS3231 rtc;
 
const char *daysOfTheWeek[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
const char *months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sen", "Oct", "Nov", "Dec"};
static DateTime now;
static float hum;  //relative humidity [%]
static float ahum; //absolute humidity [%]
static float temp; //temperature [C]
static double pres; //atm.pressure [Pa]
static bool timemarker=true;


void runButtonsCommand(uint8_t bt){
  myESPboy.myLED.setRGB(0,0,20);
  myESPboy.playTone (800, 20);

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
  


void drawled(){
  uint8_t brightness;
  if(timemarker) brightness=10;
  else brightness=4;
  myESPboy.myLED.setRGB(0,brightness,0);
  if (temp < 0) myESPboy.myLED.setRGB(0,0,brightness);
  if (temp > 30) myESPboy.myLED.setRGB(brightness,0,0);
}

 
 
void printtft(){
 
//draw date
  String toPrint="";
  myESPboy.tft.setTextColor(TFT_YELLOW);
  if (now.day() < 10) toPrint+="0";
  toPrint+=now.day();
  toPrint+=" ";
  toPrint+= months[now.month() - 1];
  toPrint+=" ";
  toPrint+=now.year();
  toPrint+=" ";
  toPrint+=daysOfTheWeek[now.dayOfTheWeek()];
  myESPboy.tft.drawString(toPrint, 20,0, 1);
 
//draw time
  myESPboy.tft.setTextColor(TFT_WHITE);
  myESPboy.tft.setTextSize(3);
  myESPboy.tft.setCursor (21, 18);
  if (now.hour() < 10) myESPboy.tft.print ("0");
  myESPboy.tft.print (now.hour());
  if(timemarker)myESPboy.tft.print (":");
  else myESPboy.tft.print ("|");
  if (now.minute() < 10) myESPboy.tft.print ("0");
  myESPboy.tft.print (now.minute());
 
//draw temp
  myESPboy.tft.setTextColor(TFT_GREEN);
  myESPboy.tft.setCursor (0, 49);
  myESPboy.tft.setTextSize(1);
  myESPboy.tft.print ("Temp   ");
  myESPboy.tft.setTextSize(2);
  myESPboy.tft.print ((int16_t)round(temp));
  myESPboy.tft.setTextSize(1);
  myESPboy.tft.print ("C");
 
//draw humidity
  myESPboy.tft.setTextColor(TFT_GREEN);
  myESPboy.tft.setCursor (0, 69);
  myESPboy.tft.setTextSize(1);
  myESPboy.tft.print ("Humid  ");
  myESPboy.tft.setTextSize(2);
  myESPboy.tft.print ((int16_t)round(hum));
  myESPboy.tft.setTextSize(1);
  myESPboy.tft.print ("% ");
  myESPboy.tft.setTextSize(2);
  myESPboy.tft.print ((int16_t)round(ahum));
  myESPboy.tft.setTextSize(1);
  myESPboy.tft.print ("g/m3");
 
//draw pressure
  myESPboy.tft.setTextColor(TFT_GREEN);
  myESPboy.tft.setCursor (0, 89);
  myESPboy.tft.setTextSize(1);
  myESPboy.tft.print ("Atm.pr ");
  myESPboy.tft.setTextSize(2);
  myESPboy.tft.print ((int16_t)round(pres));
  myESPboy.tft.setTextSize(1);
  myESPboy.tft.print ("mmHg");

}
 
 
void setup() {
//Init ESPboy
  myESPboy.begin("Weather Station");
 
//BME280 init
  myESPboy.tft.setTextColor(TFT_RED);
  myESPboy.tft.setTextSize(1);
  myESPboy.tft.setCursor(0,120);
  if (!bmx280.begin()){
    myESPboy.tft.print (F("Weather module FAILED"));
    while (1) delay(100);}
  bmx280.resetToDefaults();
  bmx280.writeOversamplingPressure(BMx280MI::OSRS_P_x16);
  bmx280.writeOversamplingTemperature(BMx280MI::OSRS_T_x16);
  bmx280.writeOversamplingHumidity(BMx280MI::OSRS_H_x16);
  bmx280.measure();
  while (!bmx280.hasValue()) delay(100);
 
//RTC init
  if (!rtc.begin()) {
    myESPboy.tft.print (F("Weather module FAILED"));
    while (1) delay(100);}

}
 
 
void loop() {
 static long count; 
 
  if (millis() > count + 2000){
     count = millis();
     timemarker=!timemarker;
     while (!bmx280.hasValue()) delay(100);
     now = rtc.now();
     bmx280.measure();
     temp = bmx280.getTemperature();
     hum = bmx280.getHumidity();
     pres = bmx280.getPressure() / 133.3d;
     ahum = 216.7f * ((hum / 100.0f) * 6.112f * exp((17.62f * temp) / (243.12f + temp)) / (273.15f + temp));
     if (hum >99) hum=99;
     if (ahum >99) ahum=99;
     myESPboy.tft.fillScreen(TFT_BLACK);
     printtft();
     drawled();
  }
 
  uint8_t bt = myESPboy.getKeys();
  if (bt) { 
    runButtonsCommand(bt); 
    myESPboy.tft.fillScreen(TFT_BLACK); 
    printtft();
    drawled();
  }
  
  delay(100);
}

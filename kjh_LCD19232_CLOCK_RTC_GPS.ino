/**********************************************************
   Title : LCD12232 DIGITAL CLOCK using RTC, GPS
   Date : 2020.03.25.
   Author : Park Yongjoon
 **********************************************************/
#include <Arduino.h>
#include <U8g2lib.h>
#include <DS1307RTC.h>
#include <TimeLib.h>
#include <TinyGPS++.h>
#include "font.h"

#define USEWIFI
//#define USEGPS

//스크롤에서 한번에 표시할 글자 수 지정
#define SCRLWIDTH 18

//스크롤 스피드 지정. 기본5 숫자가 클수록 느림
#define SCRLSPEED 8

//스크롤 글자 내용 지정
String tname = "Kim Jae-hyeon";

//----------------------------------------
//백라이트 밝기가 변하는 속도 클수록 늦음
#define BRIGHT_INTERVAL 10

#ifdef USEWIFI
#define SYNKDISPLAYTIME 3660  //GPS일경우 3초, WIFI일경우 3660초 
#endif

#ifdef USEGPS
#define SYNKDISPLAYTIME 3  //GPS일경우 3초, WIFI일경우 3660초 
#endif

#define SCREENWIDTH 192
#define SCREENHEIGHT 32

#define WidthBigNum 16
#define WidthBigNum1 8
#define WidthBigColon 4
#define HeightBigNum 32

#define WidthSmallNum 5
#define WidthSmalldot 1
#define HeightSmallNum 7

#define WidthWeek  16
#define HeightWeek  16

#define PosH1     0,0
#define PosH      12,0
#define PosColon  32,0
#define PosM10    40,0
#define PosM1     60,0

#define Posy1000    110,3
#define Posy100     117,3
#define Posy10     124,3
#define Posy1     131,3
#define Posydot   138,3
#define Posm10    141,3
#define Posm1     148,3
#define Posmdot   155,3
#define Posd10    158,3
#define Posd1     165,3

#define Poss10    82,0
#define Poss1     89,0

#define PosWeek   175, 0

#define Postxt    83,29



/////////////////여기부터 수정 금지 /////////////////////
String text;
uint8_t scrCnt = 0, scrPreCnt = 0;

//U8G2_ST7920_192X32_F_6800 u8g2(U8G2_R0, 11, 3, 12, A0, A3, 8, 7, 4, /*en=*/ 9, /*cs=*/13, /*dc=*/ 10);   // Set R/W to low!
U8G2_ST7920_192X32_F_SW_SPI u8g2(U8G2_R0, 6, 5, 4);
u8g2_uint_t offset, width;
tmElements_t tm, gm;
time_t rtcTime, gpsTime, buttonTime, pGpsTime = 0;
TinyGPSPlus gps;

boolean wifiset = false;
uint16_t cds, brightness = 255, brightSave = 255;

void setup()
{
  uint8_t i;
  for (i = 1; i < SCRLWIDTH; i++) text.concat(' ');
  text.concat(tname);

  pinMode(6, OUTPUT);
  pinMode(5, OUTPUT);
  digitalWrite(5, LOW);
  analogWrite(6, brightness);
  delay(500);
  Serial.begin(9600);

  u8g2.begin();

  RTC.read(tm);
  buttonTime = makeTime(tm);
  if (tm.Year < 48) RTC.set(1524787200); //2018년 이전이면 2018.04.27.00:00으로 셋

  //LCD TEST

  u8g2.firstPage();
  do {
    for (i = 0; i < 10; i++) {
      u8g2.drawXBMP(i * (SCREENWIDTH / 10), 0, WidthBigNum, HeightBigNum , chs_num[i]);
    }
  } while ( u8g2.nextPage());
  delay(1000);
  u8g2.firstPage();
  do {
    for (i = 0; i < 7; i++) {
      u8g2.drawXBMP(i * (SCREENWIDTH / 7), 0, WidthWeek, HeightWeek , chs_week[i]);
    }
    for (i = 0; i < 10; i++) {
      u8g2.drawXBMP(i * (SCREENWIDTH / 10), 22, WidthSmallNum, HeightSmallNum , chs_num_small[i]);
    }
  } while ( u8g2.nextPage());

  delay(1000);

  //u8g2.firstPage();
  for (i = 0; i < SCREENHEIGHT; i++) {
    do {
      u8g2.drawLine(0, i, 95, i);
      u8g2.drawLine(96, SCREENHEIGHT - 1 - i, 191, SCREENHEIGHT - 1 - i);
    } while ( u8g2.nextPage());
  }
  delay(500);
  u8g2.setFont(u8g2_font_6x13_tr); //작은글자
  u8g2.firstPage();
  do {
    u8g2.drawStr(19, 20, &tname[0]);
  } while ( u8g2.nextPage());
  delay(2000);
}


void loop() {
  rtcTime = RTC.get();
  breakTime(rtcTime, tm);

  while (Serial.available()) {
    char c;
    c = Serial.read();
    Serial.write(c);

    if (gps.encode(c)) {
      if (gps.time.isValid()) {
        gm.Year = CalendarYrToTm(gps.date.year());
        gm.Month = gps.date.month();
        gm.Day = gps.date.day();
        gm.Hour = gps.time.hour();
        gm.Minute = gps.time.minute();
        gm.Second = gps.time.second();
#ifdef USEWIFI
        if (gps.time.hour() == 88) {
          wifiset = true;
        }
        else {
          wifiset = false;
          gpsTime = makeTime(gm) + 9 * 60 * 60;
          rtcTime = gpsTime;
          pGpsTime = gpsTime;
          RTC.set(gpsTime);
        } //End of else
#endif
#ifdef USEGPS
        if ((gm.Hour != 0) || (gm.Minute != 0) || (gm.Second != 0)) { //9:00로 초기화 되는 현상 막기 위함
          gpsTime = makeTime(gm) + 9 * 60 * 60;
          if (gpsTime - pGpsTime > 1) {  //1초에 한번만 rtcSet
            rtcTime = gpsTime;
            pGpsTime = gpsTime;
            RTC.set(gpsTime);
          }
        }
#endif
      } //End of gps.time.valid()
    }
  } //End of While

  flash();
}

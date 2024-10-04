#include <Arduino.h>
#include <Wire.h>
#include <U8g2lib.h>
#include "rtc.h"

// Объявление объектов дисплеев
U8G2_SSD1306_128X64_NONAME_F_HW_I2C display1(U8G2_R0, /* reset=*/ U8X8_PIN_NONE); // Адрес 0x3C по умолчанию
U8G2_SSD1306_128X32_UNIVISION_F_HW_I2C display2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE); // Адрес 0x3C по умолчанию

void setup() {
  Serial.begin(9600);
  Serial.print("Compiled: ");
  Serial.print(__DATE__);
  Serial.println(__TIME__);

  // Инициализация RTC
  initializeRtc();

  // Установка адреса для дисплеев
  display1.setI2CAddress(0x3D * 2); // Адрес должен быть умножен на 2
  display2.setI2CAddress(0x3C * 2); // Адрес должен быть умножен на 2

  // Инициализация дисплеев
  display1.begin();
  display2.begin();
}

void loop() {
  // Обновление дисплея 1
  display1.clearBuffer();
  display1.setFont(u8g2_font_ncenB08_tr);
  display1.drawStr(0, 10, "Hello, Display 1!");
  display1.sendBuffer();

  // Получение текущего времени и даты
  RtcDateTime now = Rtc.GetDateTime();
  char buffer[25];
  snprintf(buffer, sizeof(buffer), "%02u:%02u:%02u %02u/%02u/%04u", now.Hour(), now.Minute(), now.Second(), now.Day(), now.Month(), now.Year());

  // Обновление дисплея 2
  display2.clearBuffer();
  display2.setFont(u8g2_font_ncenB08_tr);
  display2.drawStr(0, 10, buffer);
  display2.sendBuffer();

  delay(1000);
}
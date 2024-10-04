#include <Arduino.h>
#include <Wire.h>
#include <U8g2lib.h>
#include <WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include "rtc.h"

// Объявление объектов дисплеев
U8G2_SSD1306_128X64_NONAME_F_HW_I2C display1(U8G2_R0, /* reset=*/ U8X8_PIN_NONE); // Адрес 0x3C по умолчанию
U8G2_SSD1306_128X32_UNIVISION_F_HW_I2C display2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE); // Адрес 0x3C по умолчанию

// Wi-Fi настройки
const char* ssid = "Ut_far";
const char* password = "88888888";
// NTP настройки
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 0, 60000); // Обновление каждые 60 секунд

void setup() {
  Serial.begin(9600);
 Serial.println("Start");

  // Подключение к Wi-Fi
  WiFi.begin((char*)ssid, (char*)password);
  
  // Инициализация RTC
  initializeRtc();

  // Установка адреса для дисплеев
  display1.setI2CAddress(0x3D * 2); // Адрес должен быть умножен на 2
  display2.setI2CAddress(0x3C * 2); // Адрес должен быть умножен на 2

  // Инициализация дисплеев
  display1.begin();
  display2.begin();

    // Инициализация NTP клиента
  timeClient.begin();
}

void loop() {
  unsigned long currentMillis = millis();
  unsigned long previousMillis = 0; // Переменная для хранения времени последнего обновления
  const long interval = 1000; // Интервал времени между обновлениями (в миллисекундах)


  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;

    // Обновление дисплея 1
    display1.clearBuffer();
    display1.setFont(u8g2_font_ncenB08_tr);
    //display1.drawStr(0, 10, "Hello, Display 1!");
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
  }
  // Проверка подключения к Wi-Fi
  if (WiFi.status() != WL_CONNECTED) {
    display1.drawStr(0, 10,"Connecting to WiFi...");
    WiFi.begin((char*)ssid, (char*)password);
  } else {
    display1.drawStr(0, 10,"Connected to WiFi");
  }
  // Другие задачи, которые нужно выполнять в loop()
}
#include <Arduino.h>
#include <Wire.h>
#include <U8g2lib.h>
#include <U8x8lib.h>
#include <ESP8266WiFi.h>

// Объявление дисплеев
U8G2_SSD1306_128X64_NONAME_F_HW_I2C display1(U8G2_R0, /* reset=*/ U8X8_PIN_NONE); // 128x64
U8G2_SSD1306_128X32_UNIVISION_F_HW_I2C display2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE); // 128x32

// Wi-Fi настройки
const char* ssid = "Ut_far";
const char* password = "88888888";

void setup() {
  Serial.begin(9600);
  Wire.setClock(100000); // Установка I2C на 100 кГц

  Serial.println("Start");

  WiFi.begin(ssid, password);
  Serial.println("Connecting to WiFi...");
  delay(1000); // Добавлена задержка для установления соединения
  Serial.println(WiFi.localIP());


display1.setI2CAddress(0x3D * 2); // Для 128x64
display2.setI2CAddress(0x3C * 2); // Для 128x32

  // Сначала инициализируем первый дисплей
  display1.begin();
  display1.clearBuffer();

  // Потом второй дисплей
  display2.begin();
  display2.clearBuffer();
}


void loop() {
  // Обновление дисплеев в цикле
  display1.clearBuffer();
  display1.setDrawColor(1); // Установка белого цвета
  display1.setFont(u8g2_font_ncenB08_tr);
  display1.drawStr(0, 10, "Hello, Display 1!");
  display1.sendBuffer();

  display2.clearBuffer();
  display2.setDrawColor(1); // Установка белого цвета
  display2.setFont(u8g2_font_ncenB08_tr);
  display2.drawStr(0, 10, "Hello, Display 2!");
  display2.sendBuffer();

  delay(1000); // Задержка для обновления дисплеев
}
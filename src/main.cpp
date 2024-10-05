#include <Arduino.h>
#include <Wire.h>
#include <U8g2lib.h>
#include <U8x8lib.h>
#include <ESP8266WiFi.h>

// Объявление дисплеев
U8G2_SSD1306_128X64_NONAME_F_HW_I2C display1(U8G2_R0, /* reset=*/U8X8_PIN_NONE);    // 128x64
U8G2_SSD1306_128X32_UNIVISION_F_HW_I2C display2(U8G2_R0, /* reset=*/U8X8_PIN_NONE); // 128x32

// Габариты дисплеев
#define DISPLAY1_WIDTH 128 
#define DISPLAY1_HEIGHT 64
#define DISPLAY2_WIDTH 128
#define DISPLAY2_HEIGHT 32

// Wi-Fi настройки
const char *ssid = "Ut_far";
const char *password = "88888888";
// Настройки статического IP
IPAddress local_IP(192, 168, 0, 104);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);

// Объявление функции displaydraw
void displaydraw();

void setup()
{
  Serial.begin(9600);
  Wire.setClock(100000); // Установка I2C на 100 кГц
  Serial.println("Start");
  // Установка статического IP
  if (!WiFi.config(local_IP, gateway, subnet))
  {    Serial.println("STA Failed to configure");  }
  // Подключение к Wi-Fi
  WiFi.begin(ssid, password);
  Serial.println("Connecting to WiFi...");
  // Ожидание подключения к Wi-Fi
  while (WiFi.status() != WL_CONNECTED)
  {    delay(500);    Serial.print(".");  }
  Serial.println();
  Serial.println("Connected to WiFi");
  Serial.println(WiFi.localIP());
  // Устанавлием адреса I2C для дисплеев
  display1.setI2CAddress(0x3D * 2); // Для 128x64
  display2.setI2CAddress(0x3C * 2); // Для 128x32
  // Инициализируем первый дисплей
  display1.begin();
  display1.setDrawColor(1); // Установка белого цвета
  display1.clearBuffer();
  // Потом второй дисплей
  display2.begin();
  display2.setDrawColor(1); // Установка белого цвета
  display2.clearBuffer();
}

void loop()
{

  static unsigned long lastUpdateTime = 0;
  unsigned long currentTime = millis();
  if (currentTime - lastUpdateTime > 1000)
  {    lastUpdateTime = currentTime;   
   displaydraw();  }
}
// Вывод на дисплеи
void displaydraw()
{
    IPAddress ip = WiFi.localIP();
  String ipStr = ip.toString();
  const char *ipChar = ipStr.c_str();
  display1.clearBuffer();
  display1.setFont(u8g2_font_ncenB14_tr);
  if (WiFi.status() == WL_CONNECTED)
  {    display1.drawStr(0, 14, "Connected"); 
  display1.drawStr(0, (DISPLAY1_HEIGHT/2)+7, ipChar);  }
  else
  {    display1.drawStr(0, 10, "Not connected");  }
  display1.sendBuffer();
  display2.clearBuffer();
  display2.setFont(u8g2_font_ncenB08_tr);
  display2.drawStr(0, 10, "Hello, Display 2!");
  display2.sendBuffer();
}
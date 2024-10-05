#include <Arduino.h>
#include <Wire.h>
#include <U8g2lib.h>
#include <U8x8lib.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>

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
// Переменные для хранения Wi-Fi точки доступа
const char *ap_ssid = "ESP8266";
const char *ap_password = "12345679";

ESP8266WebServer server(80);
// Настройки статического IP
IPAddress local_IP(192, 168, 0, 104);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);
// Обработчик для корневого пути
void handleRoot() {
  server.send(200, "text/html", 
    "<form action='/submit1'>"
    "<label for='input1'>SSID:</label>"
    "<input type='text' id='input1' name='input1'>"
    "<input type='submit' value='Submit'>"
    "</form>"
    "<form action='/submit2'>"
    "<label for='input2'>Password:</label>"
    "<input type='text' id='input2' name='input2'>"
    "<input type='submit' value='Submit'>"
    "</form>"
  );
}
// Обработчик для первой формы
void handleSubmit1()
{
  String input1 = server.arg("input1");
  server.send(200, "text/plain", "Received input1: " + input1);
}
// Обработчик для второй формы
void handleSubmit2()
{
  String input2 = server.arg("input2");
  server.send(200, "text/plain", "Received input2: " + input2);
}

// Объявление функции displaydraw
void displaydraw();
void accessPoint();

void setup()
{
  int failedAttemps = 0;
  Serial.begin(9600);
  Wire.setClock(100000); // Установка I2C на 100 кГц
  Serial.println("Start");
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

  // Установка статического IP
  if (!WiFi.config(local_IP, gateway, subnet))
  {
    Serial.println("STA Failed to configure");
  }
  // Подключение к Wi-Fi
  WiFi.begin(ssid, password);
  Serial.println("Connecting to WiFi...");
  // Ожидание подключения к Wi-Fi
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
    failedAttemps++;
    if (failedAttemps > 3)
    {
      Serial.println();
      Serial.println("Failed to connect to WiFi");
      break;
    }
  }
  if (WiFi.status() == WL_CONNECTED)
  {
    Serial.println("Connected to WiFi");
    Serial.println(WiFi.localIP());
  }
  display1.sendBuffer();
  display2.sendBuffer();
}

void loop()
{

  static unsigned long lastUpdateTime = 0;
  unsigned long currentTime = millis();
  if (currentTime - lastUpdateTime > 1000)
  {
    lastUpdateTime = currentTime;
    display1.clearBuffer();
    display2.clearBuffer();
    accessPoint();
    displaydraw();
    server.handleClient();
    display1.sendBuffer();
    display2.sendBuffer();
  }
}
// Вывод на дисплеи
void displaydraw()
{

  display1.setFont(u8g2_font_ncenB10_tr);
  if (WiFi.status() == WL_CONNECTED)
  {
    display1.drawStr(0, 12, "Connected to WiFi");
    display1.drawStr(0, 24, WiFi.localIP().toString().c_str());
  }
  else
  {
    display1.setFont(u8g2_font_ncenB08_tr);
    display1.drawStr(12, 14, "Failed to connect");
  }

  display2.setFont(u8g2_font_ncenB10_tr);
  display2.drawStr(0, 14, "Hello, Display 2!");
}

void accessPoint()
{
  static int firstTimeACP = 0;
  if (WiFi.status() != WL_CONNECTED)
  {
    if (firstTimeACP == 0)
    {
      WiFi.disconnect();
      WiFi.mode(WIFI_AP);
      WiFi.softAPConfig(local_IP, gateway, subnet);
      WiFi.softAP(ap_ssid, ap_password);
      Serial.println("Access Point started");
      Serial.println("IP Address:");
      Serial.println(WiFi.softAPIP());
      // Настройка веб-сервера
      server.on("/", handleRoot);
      server.on("/submit1", handleSubmit1);
      server.on("/submit2", handleSubmit2);
      server.begin();
      Serial.println("Web server started");
      firstTimeACP = 1;
    }

    display1.setFont(u8g2_font_ncenB08_tr);
    display1.drawStr(8, 36, "Access Point started");
    display1.setFont(u8g2_font_ncenB12_tr);
    display1.drawStr(4, 56, WiFi.softAPIP().toString().c_str());
  }
}

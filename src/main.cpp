#include <Arduino.h>
#include <Wire.h>
#include <U8g2lib.h>
#include <U8x8lib.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include <EEPROM.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <RtcDS1302.h>
#include <AHT10.h>
#include <iarduino_Pressure_BMP.h>

// Define the Time structure if not included in the library
struct Time
{
  int year;
  int month;
  int date;
  int hour;
  int min;
  int sec;
};

// Объявление функций
void displaydraw();
void accessPoint();
void handleSubmit();
void handleRoot();
void updateClock();
void syncTimeWithAPI();
void brightnessControl();
void tempandhum();

// Объявление дисплеев
U8G2_SSD1306_128X64_NONAME_F_HW_I2C display1(U8G2_R0, /* reset=*/U8X8_PIN_NONE);    // Дисплей 128x64
U8G2_SSD1306_128X32_UNIVISION_F_HW_I2C display2(U8G2_R0, /* reset=*/U8X8_PIN_NONE); // Дисплей 128x32
AHT10 aht10;                                                                        // Датчик температуры и влажности
iarduino_Pressure_BMP bmp;                                                          // Датчик давления

// Габариты дисплеев
#define DISPLAY1_WIDTH 128
#define DISPLAY1_HEIGHT 64
#define DISPLAY2_WIDTH 128
#define DISPLAY2_HEIGHT 32

// Пины для подключения DS1302
const int RST_PIN = 15;
const int DAT_PIN = 13;
const int CLK_PIN = 12;
// Создание объекта ThreeWire и DS1302
ThreeWire myWire(DAT_PIN, CLK_PIN, RST_PIN);
RtcDS1302<ThreeWire> rtc(myWire);

// Переменные для хранения Wi-Fi точки доступа
const char *ap_ssid = "ESP8266";
const char *ap_password = "12345679";
bool wasConnectedToWiFi = false;

ESP8266WebServer server(80);
// Настройки NTP
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 0, 60000); // 0 - смещение по времени (в секундах), 60000 - интервал обновления (в миллисекундах)

// Обработчик для главной страницы
void handleRoot()
{
  String html = "<!DOCTYPE html><html><head><title>WiFi Setup</title>";
  html += "<style>";
  html += "body { background-color: #121212; color: #ffffff; display: flex; justify-content: center; align-items: center; height: 100vh; margin: 0; font-family: Arial, sans-serif; }";
  html += ".container { text-align: center; width: 90%; max-width: 600px; }";
  html += "input { display: block; margin: 20px auto; padding: 15px; width: 90%; max-width: 500px; font-size: 1.2em; }";
  html += "button { padding: 15px 30px; background-color: #1e88e5; color: #ffffff; border: none; cursor: pointer; font-size: 1.2em; }";
  html += "button:hover { background-color: #1565c0; }";
  html += "</style></head><body>";
  html += "<div class='container'>";
  html += "<h1>WiFi Setup</h1>";
  html += "<form action='/submit' method='POST'>";
  html += "<input type='text' name='ssid' placeholder='SSID' required>";
  html += "<input type='password' name='password' placeholder='Password' required>";
  html += "<button type='submit'>Submit</button>";
  html += "</form></div></body></html>";
  server.send(200, "text/html", html);
}
// Обработчик для маршрута /submit
void handleSubmit()
{
  if (server.hasArg("ssid") && server.hasArg("password"))
  {
    String ssid = server.arg("ssid");
    String password = server.arg("password");

    // Ensure the EEPROM has enough space for the SSID and password
    if (ssid.length() + password.length() + 2 > 512)
    {
      server.send(500, "text/html", "<h1>Data too large for EEPROM</h1>");
      return;
    }

    // Write the lengths of the SSID and password to EEPROM
    EEPROM.write(0, ssid.length());
    EEPROM.write(1, password.length());

    // Write the SSID to EEPROM starting at address 2
    for (size_t i = 0; i < ssid.length(); i++)
    {
      EEPROM.write(2 + i, ssid[i]);
    }

    // Write the password to EEPROM starting after the SSID
    for (size_t i = 0; i < password.length(); i++)
    {
      EEPROM.write(2 + ssid.length() + i, password[i]);
    }

    EEPROM.commit();
    server.send(200, "text/html", "<h1>Data received, restart controller</h1>");
  }
  else
  {
    server.send(400, "text/html", "<h1>Invalid Request</h1>");
  }
}

// Настройки статического IP
IPAddress local_IP(192, 168, 0, 105);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);

void setup()
{
  int failedAttemps = 0;
  Serial.begin(9600);
  EEPROM.begin(512);
  rtc.Begin();
  Wire.setClock(100000); // Установка I2C на 100 кГц

  Serial.println("Start");
  // Read the lengths of the SSID and password from EEPROM
  int ssidLength = EEPROM.read(0);
  int passwordLength = EEPROM.read(1);

  // Read the SSID from EEPROM
  String storedSSID = "";
  for (int i = 0; i < ssidLength; i++)
  {
    storedSSID += char(EEPROM.read(2 + i));
  }

  // Read the password from EEPROM
  String storedPassword = "";
  for (int i = 0; i < passwordLength; i++)
  {
    storedPassword += char(EEPROM.read(2 + ssidLength + i));
  }
  EEPROM.end();
  Serial.println("Stored SSID:" + storedSSID);
  Serial.println("Stored Password:" + storedPassword);

  // Подключение к Wi-Fi по сохраненным данным
  WiFi.begin(storedSSID.c_str(), storedPassword.c_str());

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

  Serial.println("Connecting to WiFi");
  // Ожидание подключения к Wi-Fi
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
    failedAttemps++;
    display1.clearBuffer();
    display1.setFont(u8g2_font_ncenB08_tr);
    display1.drawStr(0, 14, "Connecting to WiFi");
    display1.drawStr(0, 28, "Failed attemps:");
    display1.drawStr(86, 28, String(failedAttemps).c_str());
    display1.sendBuffer();
    if (failedAttemps > 9)
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
  // Инициализация датчика AHT10
  if (!aht10.begin())
  {
    Serial.println("Не удалось инициализировать датчик AHT10!");
    while (1)
      ;
  }
  else
  {
    Serial.println("Датчик AHT10 инициализирован.");
  }
  // Инициализация датчика BMP
  if (!bmp.begin())
  {
    Serial.println("Не удалось инициализировать датчик BMP!");
  }
  else
  {
    Serial.println("Датчик BMP инициализирован.");
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
    // Проверка состояния Wi-Fi и обновление переменной wasConnectedToWiFi
    if (WiFi.status() == WL_CONNECTED)
    {
      wasConnectedToWiFi = true;
    }
    lastUpdateTime = currentTime;
    display1.clearBuffer();
    display2.clearBuffer();
    accessPoint();
    displaydraw();
    updateClock();
    brightnessControl();
    tempandhum();
    server.handleClient();
    display1.sendBuffer();
    display2.sendBuffer();
  }
}
// Вывод на дисплеи
void displaydraw()
{
  display1.setFont(u8g2_font_ncenB08_tr);
  if (WiFi.status() == WL_CONNECTED && millis() < 10000)
  {
    if (WiFi.status() == WL_CONNECTED)
    {
      display1.drawStr(0, 14, "Connected to WiFi");
      display1.drawStr(0, 28, WiFi.localIP().toString().c_str());
    }
    else
    {
      display1.setFont(u8g2_font_ncenB08_tr);
      display1.drawStr(16, 14, "Failed to connect");
    }
  }
  if (millis() > 10000)
  {
    display1.setFont(u8g2_font_unifont_t_symbols);
    if (WiFi.status() == WL_CONNECTED)
    {
      display1.drawUTF8(112, 12, "\u2714"); // Использование символа галочки
    }
    else
    {
      display1.drawUTF8(112, 12, "\u274C"); // Использование символа крестика
    }
  }
}
// Режим точки доступа
static bool firstTimeACP = 0;
void accessPoint()
{

  if (WiFi.status() != WL_CONNECTED && !wasConnectedToWiFi)
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
      // Настройка маршрутов
      server.on("/", handleRoot);
      server.on("/submit", HTTP_POST, handleSubmit); // Добавляем маршрут /submit
      server.begin();
      Serial.println("HTTP server started");

      firstTimeACP = 1;
    }

    display1.setFont(u8g2_font_ncenB08_tr);
    display1.drawStr(8, 36, "Access Point started");
    display1.setFont(u8g2_font_ncenB12_tr);
    display1.drawStr(4, 56, WiFi.softAPIP().toString().c_str());
    display2.setFont(u8g2_font_ncenB08_tr);
    display2.drawStr(0, 10, "Connect to WiFi");
    display2.drawStr(0, 20, ("SSID: " + String(ap_ssid)).c_str());
    display2.drawStr(0, 30, ("Password: " + String(ap_password)).c_str());
    server.handleClient();
  }
}

// Объявляем переменные
unsigned long currentTime = 0;  // Текущее время
unsigned long lastSyncTime = 0; // Время последней успешной синхронизации
bool TimeSynced = false;        // Указывает, была ли синхронизация успешной
bool firstSync = false;         // Отслеживает выполнение первой синхронизации
                                // Определения дня недели и месяца
const char *getDayOfWeek(int year, int month, int day)
{
  // Используем алгоритм Томохико Сакамото для вычисления дня недели
  static int t[] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
  if (month < 3)
  {
    year -= 1;
  }
  int dayOfWeek = (year + year / 4 - year / 100 + year / 400 + t[month - 1] + day) % 7;

  // Массив дней недели
  const char *days[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
  return days[dayOfWeek];
}
const char *getMonthName(int month)
{
  // Массив месяцев
  const char *months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
  return months[month - 1];
}
// Обновление времени
void updateClock()
{
  currentTime = millis(); // Получаем текущее время с момента старта контроллера
  // Проверяем подключение к WiFi
  if (WiFi.status() == WL_CONNECTED)
  {
    // Выполняем первую синхронизацию или повторяем при неудаче, каждые 5 секунд
    if ((!firstSync || !TimeSynced) && currentTime - lastSyncTime >= 5000)
    {
      syncTimeWithAPI();          // Пытаемся синхронизировать время
      lastSyncTime = currentTime; // Обновляем время последней попытки синхронизации
    }
    // Если время успешно синхронизировано, синхронизируем раз в 5 минут
    else if (TimeSynced && currentTime - lastSyncTime >= 300000)
    {
      syncTimeWithAPI();          // Синхронизируем время
      lastSyncTime = currentTime; // Обновляем время последней синхронизации
    }
  }
  // Получаем текущее время с RTC модуля
  RtcDateTime t = rtc.GetDateTime();

  // Устанавливаем шрифт для дисплея
  display2.setFont(u8g2_font_ncenB10_tr); // Шрифт для времени

  // Формируем строку времени в формате HH:MM:SS
  char buffer[20];
  snprintf(buffer, sizeof(buffer), "%02d:%02d:%02d", t.Hour(), t.Minute(), t.Second());
  display2.clearBuffer();                                  // Очищаем экран перед выводом
  display2.drawStr((DISPLAY2_WIDTH / 2) - 28, 12, buffer); // Время по центру экрана

  // Получаем название дня недели и месяца
  const char *dayOfWeek = getDayOfWeek(t.Year(), t.Month(), t.Day());
  const char *monthName = getMonthName(t.Month());

  // Увеличиваем шрифт для даты
  display2.setFont(u8g2_font_ncenB10_tr);
  // Формируем строку для дня недели, числа дня и месяца
  snprintf(buffer, sizeof(buffer), "%s, %02d %s", dayOfWeek, t.Day(), monthName);
  display2.drawStr(16, 28, buffer); // День недели, дата и месяц

  // Устанавливаем меньший шрифт для номера месяца
  display2.setFont(u8g2_font_5x7_tr);                    // Меньший шрифт для числа месяца
  snprintf(buffer, sizeof(buffer), "(%02d)", t.Month()); // Номер месяца в скобках
  display2.drawStr(105, 24, buffer);                     // Отображаем номер месяца в скобках
}

// Синхронизация времени с API worldtimeapi.org
void syncTimeWithAPI()
{
  HTTPClient http;
  WiFiClient client;

  // Запрос времени с API
  http.begin(client, "http://worldtimeapi.org/api/timezone/Europe/Kyiv");
  int httpCode = http.GET();

  // Если ответ успешен
  if (httpCode == HTTP_CODE_OK)
  {
    String payload = http.getString(); // Получаем ответ API
    DynamicJsonDocument doc(1024);     // Создаем JSON-документ
    deserializeJson(doc, payload);     // Десериализуем JSON

    const char *datetime = doc["datetime"]; // Получаем строку с датой и временем
    int year, month, day, hour, minute, second;

    // Парсим строку datetime в отдельные компоненты
    sscanf(datetime, "%d-%d-%dT%d:%d:%d", &year, &month, &day, &hour, &minute, &second);

    // Устанавливаем новое время для RTC модуля
    RtcDateTime newTime(year, month, day, hour, minute, second);
    rtc.SetDateTime(newTime); // Обновляем время RTC

    // Успешная синхронизация
    TimeSynced = true;       // Отмечаем, что синхронизация успешна
    firstSync = true;        // Первая синхронизация выполнена
    lastSyncTime = millis(); // Обновляем время последней успешной синхронизации

    Serial.println("Time synchronized successfully with API");
  }
  else
  {
    // Если не удалось синхронизировать время
    TimeSynced = false; // Отмечаем, что синхронизация не удалась
    Serial.println("Failed to get time from API");
  }

  http.end(); // Завершаем HTTP-запрос
}

// Яркость дисплеев
void brightnessControl()
{
  const int analogPin = A0; // Пин, к которому подключен фоторезистор
  const float R2 = 10000.0; // Резистор 10 кОм
  const float Vin = 3.3;    // Напряжение питания 3.3В

  float Vout = analogRead(analogPin) * (Vin / 1024.0); // Преобразование в напряжение (0-3.3В)

  // Рассчитаем сопротивление фоторезистора
  float R1 = (R2 * (Vin - Vout)) / Vout; // Сопротивление фоторезистора

  // Масштабируем значение для яркости дисплея
  int brightness1 = map(R1, 0, 34000, 255, 50);   // Изменить минимальное и максимальное значения для первого дисплея
  brightness1 = constrain(brightness1, 100, 255); // Ограничиваем значение яркости в диапазоне 50-255

  int brightness2 = map(R1, 8000, 20000, 255, 0); // Изменить минимальное и максимальное значения для второго дисплея
  brightness2 = constrain(brightness2, 0, 200);   // Ограничиваем значение яркости в диапазоне 0-255

  display1.setContrast(brightness1);
  display2.setContrast(brightness2);

  // Serial.print("Resistance: ");
  // Serial.print(R1);
  // Serial.print(" Ohm, Brightness1: ");
  // Serial.print(brightness1);
  // Serial.print(", Brightness2: ");
  // Serial.println(brightness2);
}

void tempandhum()
{
  float temperature = aht10.readTemperature(); // Read temperature from AHT10 sensor
  float humidity = aht10.readHumidity();       // Read humidity from AHT10 sensor
  float pressure = bmp.pressure;
  temperature = bmp.temperature;
  if (millis() > 10000 && firstTimeACP == 0)
  {
    // Calculate average temperature from AHT10 and BMP sensors
    float avgTemperature = aht10.readTemperature();

    // Prepare strings for display
    char tempStr[10];
    char humStr[10];
    char pressureStr[10];
    snprintf(tempStr, sizeof(tempStr), "%.1f", avgTemperature);
    snprintf(humStr, sizeof(humStr), "%.1f", humidity);
    snprintf(pressureStr, sizeof(pressureStr), "%.1f", pressure); // Convert pressure to hPa

    // Display temperature, humidity, and pressure with labels
    display1.setFont(u8g2_font_ncenB08_tr);
    // Display temperature
    display1.drawStr(0, 10, "Temp:");
    int tempWidth = display1.getStrWidth(tempStr); // Вычисляем ширину строки с температурой
    display1.drawStr(40, 10, tempStr);             // Температура
    display1.setFont(u8g2_font_unifont_t_symbols);
    display1.drawUTF8(40 + tempWidth + 1, 10, "\u00B0"); // Добавляем символ градуса "°"
    display1.setFont(u8g2_font_ncenB08_tr);
    display1.drawStr(40 + tempWidth + 6, 10, "C"); // Добавляем букву "C" рядом с градусом
    // Display humidity
    display1.setFont(u8g2_font_ncenB08_tr);
    display1.drawStr(0, 26, "Hum:");
    int humWidth = display1.getStrWidth(humStr); // Вычисляем ширину строки с влажностью
    display1.drawStr(40, 26, humStr);            // Влажность
    display1.setFont(u8g2_font_unifont_t_symbols);
    display1.drawUTF8(40 + humWidth + 1, 26, "\u0025"); // Добавляем символ процента "%"
    // Display pressure
    display1.setFont(u8g2_font_ncenB08_tr);
    display1.drawStr(0, 42, "Pres:");
    int pressureWidth = display1.getStrWidth(pressureStr); // Вычисляем ширину строки с давлением
    display1.drawStr(40, 42, pressureStr);                 // Давление
    display1.drawStr(40 + pressureWidth + 1, 42, "hPa");   // Добавляем единицу измерения давления "hPa"
  }
}
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
#include <UniversalTelegramBot.h>
#include "config.h"

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
void displaydraw();          // Функция отрисовки дисплея
void accessPoint();          // Функция создания точки доступа
void handleSubmit();         // Обработчик для маршрута /submit
void handleRoot();           // Обработчик для главной страницы
void updateClock();          // Функция обновления времени
void syncTimeWithAPI();      // Функция синхронизации времени с API
void brightnessControl();    // Функция управления яркостью дисплеев
void tempandhum();           // Функция получения температуры и влажности
void weather();              // Функция получения погоды
String weatherjsonget();     // Функция получения погоды в формате JSON
void displayWeather();       // Функция отображения погоды на дисплее
String line;                 // Переменная для хранения строки
void accesspointcondition(); // Функция для проверки условия для включения точки доступа
void startTelegramBot();     // Функция для запуска бота
void handleNewMessages(int numNewMessages);

// Объявление дисплеев и датчиков
U8G2_SSD1306_128X64_NONAME_F_HW_I2C display1(U8G2_R0, /* reset=*/U8X8_PIN_NONE);    // Дисплей 128x64
U8G2_SSD1306_128X32_UNIVISION_F_HW_I2C display2(U8G2_R0, /* reset=*/U8X8_PIN_NONE); // Дисплей 128x32
AHT10 aht10;                                                                        // Датчик температуры и влажности
iarduino_Pressure_BMP bmp;                                                          // Датчик давления
WiFiClientSecure secured_client;
UniversalTelegramBot bot(BOT_TOKEN, secured_client); // Бот для отправки сообщений в Telegram

// Габариты дисплеев
#define DISPLAY1_WIDTH 128
#define DISPLAY1_HEIGHT 64
#define DISPLAY2_WIDTH 128
#define DISPLAY2_HEIGHT 32

// Пины для подключения кнопок
const int button1 = 0;  // D3
const int button2 = 2;  // D4
const int button3 = 14; // D5
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
static bool workingACP = 0;
unsigned long lastTimeBotRan;

// Данные для подключения к интернету
ESP8266WebServer server(80);
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
  EEPROM.begin(512);
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
    String response = "<!DOCTYPE html><html><head><title>Data received, restarting...</title>";
    response += "<style>";
    response += "body { background-color: #000000; color: #ffffff; display: flex; justify-content: center; align-items: center; height: 100vh; margin: 0; font-family: Arial, sans-serif; }";
    response += "h1 { font-size: 3em; }";
    response += "</style></head><body>";
    response += "<h1>Data received, restarting...</h1>";
    response += "</body></html>";
    server.send(200, "text/html", response);
    Serial.println("Data received");
    Serial.println("SSID: " + ssid);
    Serial.println("Password: " + password);
    delay(5000);
    ESP.restart();
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
X509List cert(TELEGRAM_CERTIFICATE_ROOT);

// Функция старта бота
void startTelegramBot()
{
  // Установка времени для TLS
  configTime(0, 0, "pool.ntp.org");
  secured_client.setTrustAnchors(&cert);

  bot.sendMessage(CHAT_ID, "Bot is online and ready! Press /start to continue", "");
  // Устанавливаем команды бота
  const String commands = F("["
                            "{\"command\":\"RoomStatus\", \"description\":\"Room temp and hum\"},"
                            "{\"command\":\"test1\", \"description\":\"Send Pc sleep\"},"
                            "{\"command\":\"test2\", \"description\":\"Turn on/off\"},"
                            "{\"command\":\"test3\", \"description\":\"Get humidity\"}"
                            "]");
  bot.setMyCommands(commands); // Устанавливаем команды для бота
}

// Функция для обработки новых сообщений
void handleNewMessages(int numNewMessages)
{
  for (int i = 0; i < numNewMessages; i++)
  {
    String chat_id = String(bot.messages[i].chat_id);
    String text = bot.messages[i].text;

    if (text == "/start")
    {
      String welcome = "Use the following commands:\n";
      welcome += "/RoomStatus - Get room temperature and humidity\n";
      welcome += "/test1 - Send PC to sleep\n";
      welcome += "/test2 - Turn on/off\n";
      welcome += "/test3 - Get humidity\n";
      bot.sendMessage(chat_id, welcome, "");
    }

    if (text == "/RoomStatus")
    {
      float temperature = aht10.readTemperature(); // Read temperature from AHT10 sensor
      float humidity = aht10.readHumidity();       // Read humidity from AHT10 sensor
      String roomStatus = "Room temperature: " + String(temperature, 1) + "°C\n";
      roomStatus += "Room humidity: " + String(humidity, 1) + "%\n";
      bot.sendMessage(chat_id, roomStatus, "");
    }

    if (text == "/test1")
    {
    }

    if (text == "/test2")
    {
    }

    if (text == "/test3")
    {
    }
  }
}

// Стартовая функция
void setup()
{
  pinMode(button1, INPUT_PULLUP);
  pinMode(button2, INPUT_PULLUP);
  pinMode(button3, INPUT_PULLUP);
  int failedAttemps = 0;
  Serial.begin(9600);
  EEPROM.begin(512);
  // EEPROM.write(0, 0);
  // EEPROM.write(1, 0);
  // EEPROM.commit();
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
      workingACP = 1;
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
  startTelegramBot();
}

void loop()
{
  static unsigned long lastUpdateTime = 0;
  unsigned long currentTime = millis();
  static unsigned long lastUpdateTime1 = 0;
  unsigned long currentTime1 = millis();
  if (currentTime - lastUpdateTime > 1000)
  {
    lastUpdateTime = currentTime;
    display1.clearBuffer();
    display2.clearBuffer();
    accessPoint();
    displaydraw();
    updateClock();
    brightnessControl();
    tempandhum();
    weather();
    displayWeather();
    int numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    while (numNewMessages)
    {
      handleNewMessages(numNewMessages);
      numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    }
    server.handleClient();
    display1.sendBuffer();
    display2.sendBuffer();
    // Serial.println("workingACP: " + String(workingACP));
  }
  if (currentTime1 - lastUpdateTime1 > 5)
  {
    lastUpdateTime1 = currentTime1;
    accesspointcondition();

    if (digitalRead(button3) == LOW)
    {
      Serial.println("Button pressed");
    }
  }
}
// Вывод на дисплеи
void displaydraw()
{
  display1.setFont(u8g2_font_ncenB08_tr);
  if (WiFi.status() == WL_CONNECTED && millis() < 8800)
  {
    if (WiFi.status() == WL_CONNECTED)
    {
      display1.drawStr(0, 14, "Connected to WiFi");
      display1.drawStr(0, 28, WiFi.localIP().toString().c_str());
    }
    // else
    // {
    //   display1.setFont(u8g2_font_ncenB08_tr);
    //   display1.drawStr(16, 14, "Failed to connect");
    // }
  }
  if (millis() > 9000)
  {
    display1.setFont(u8g2_font_unifont_t_symbols);
    if (WiFi.status() == WL_CONNECTED)
    {
      display1.drawUTF8(98, 20, "\u2714"); // Использование символа галочки
    }
    if (WiFi.status() != WL_CONNECTED && workingACP == 0)
    {
      display1.drawUTF8(98, 20, "\u2718"); // Использование символа крестика
    }
  }
}
// Режим точки доступа
void accessPoint()
{
  static bool accessPointInitialized = false; // Флаг для отслеживания инициализации точки доступа

  if (workingACP == 1 && !accessPointInitialized)
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

    accessPointInitialized = true; // Устанавливаем флаг, чтобы точка доступа не инициализировалась повторно
  }
  else if (workingACP == 0 && accessPointInitialized)
  {
    WiFi.softAPdisconnect(true);    // Отключаем точку доступа
    accessPointInitialized = false; // Сбрасываем флаг инициализации
    Serial.println("Access Point stopped");
  }

  if (accessPointInitialized)
  {
    server.handleClient();
  }
  if (workingACP == 1)
  {

    display1.setFont(u8g2_font_ncenB08_tr);
    display1.drawStr(8, 12, "Access Point started");
    display1.setFont(u8g2_font_ncenB12_tr);
    display1.drawStr(4, 28, WiFi.softAPIP().toString().c_str());
    // Обновляем позицию текста
    /// Объявляем позицию текста как int для точности при движении
    static int textPosX1 = 12;                               // Начальная позиция текста
    const char *scrollText = "Press any button to continue"; // Текст бегущей строки
    int textWidth = display1.getStrWidth(scrollText);        // Ширина текста
    int spacing = 64;                                        // Устанавливаем расстояние между строками (меньше текстового пространства)
    // Обновляем позицию для бегущей строки
    textPosX1 -= 4; // Скорость движения текста
    // Если первая строка полностью ушла за левый край экрана
    if (textPosX1 < -textWidth)
    {
      textPosX1 += textWidth - spacing; // Смещаем её за вторую строку с учётом уменьшенного расстояния
    }
    // Устанавливаем шрифт
    display1.setFont(u8g2_font_ncenB08_tr);
    // Рисуем первую строку на экране
    display1.drawStr(textPosX1, 48, scrollText);
    // Рисуем вторую строку сразу после первой, с уменьшенным расстоянием между ними
    display1.drawStr(textPosX1 + textWidth - spacing, 48, scrollText);
    display2.setFont(u8g2_font_ncenB08_tr);
    display2.drawStr(0, 10, "Connect to WiFi");
    display2.drawStr(0, 20, ("SSID: " + String(ap_ssid)).c_str());
    display2.drawStr(0, 30, ("Password: " + String(ap_password)).c_str());
  }
}

void accesspointcondition()
{
  if (WiFi.status() != WL_CONNECTED && workingACP == 1)
  {
    if (digitalRead(button1) == LOW || digitalRead(button2) == LOW || digitalRead(button3) == LOW)
    {
      Serial.println("Button pressed");
      workingACP = 0;
    }
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
  // Проверяем, не включен ли режим точки доступа
  if (workingACP == 0)
  {

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
}

// Синхронизация времени с API worldtimeapi.org
void syncTimeWithAPI()
{
  HTTPClient http;
  WiFiClient client;

  // Запрос времени с API
  http.begin(client, TimeApi);
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
}

// Получение температуры и влажности
void tempandhum()
{
  float temperature = aht10.readTemperature(); // Read temperature from AHT10 sensor
  float humidity = aht10.readHumidity();       // Read humidity from AHT10 sensor
  float pressure = bmp.pressure;
  temperature = bmp.temperature;
  if (millis() > 9000 && workingACP == 0)
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

    // Display pressure
    display1.drawStr(0, 10, "Pres:");
    int pressureWidth = display1.getStrWidth(pressureStr); // Вычисляем ширину строки с давлением
    display1.drawStr(40, 10, pressureStr);                 // Давление
    display1.drawStr(44 + pressureWidth + 1, 10, "hPa");   // Добавляем единицу измерения давления "hPa"

    // Display humidity
    display1.drawStr(0, 26, "Hum:");
    int humWidth = display1.getStrWidth(humStr); // Вычисляем ширину строки с влажностью
    display1.drawStr(40, 26, humStr);            // Влажность
    display1.setFont(u8g2_font_unifont_t_symbols);
    display1.drawUTF8(44 + humWidth + 1, 26, "\u0025"); // Добавляем символ процента "%"

    // Display temperature
    display1.setFont(u8g2_font_ncenB08_tr);
    display1.drawStr(0, 42, "Temp:");
    int tempWidth = display1.getStrWidth(tempStr); // Вычисляем ширину строки с температурой
    display1.drawStr(40, 42, tempStr);             // Температура

    if (WiFi.status() == !WL_CONNECTED)
    {
      display1.setFont(u8g2_font_unifont_t_symbols);
      display1.drawUTF8(40 + tempWidth + 1, 42, "\u00B0"); // Добавляем символ градуса "°"
      display1.setFont(u8g2_font_ncenB08_tr);
      display1.drawStr(44 + tempWidth + 6, 42, "C"); // Добавляем букву "C" рядом с градусом
    }
  }
}

// Получение погоды
void weather()
{
  DynamicJsonDocument doc(2000);                           /// буфер на 2000 символов
  String line = weatherjsonget();                          // объявляем переменную line и получаем результат из функции weatherjsonget
  DeserializationError error = deserializeJson(doc, line); // скармиваем String
  if (error)
  {
    // Serial.println("deserializeJson() failed"); // если ошибка, сообщаем об этом
    return; // и запускаем заного
  }
}
String weatherjsonget()
{
  static unsigned long lastWeatherUpdateTime = 0;
  unsigned long currentTime = millis();

  // Check if 10 seconds have passed since the controller started or if a minute has passed since the last update
  if ((currentTime > 10000 && lastWeatherUpdateTime == 0) || (currentTime - lastWeatherUpdateTime >= 30000))
  {
    const char *host = "api.openweathermap.org"; // Declare the host
    const int httpPort = 80;                     // Declare and initialize the httpPort variable
    // Use WiFiClient class to create TCP connections
    WiFiClient client;
    if (!client.connect(host, httpPort))
    {
      // Serial.println("connection failed");
      return "";
    }

    String url = String("/data/2.5/weather?id=") + Cityid + "&appid=" + WeatherApiKey;
    client.println("GET " + url + " HTTP/1.1");
    client.println("Host: api.openweathermap.org");
    client.println("Connection: close");
    client.println();

    delay(1500);
    // Read all the lines of the reply from server and print them to Serial
    while (client.available())
    {
      line = client.readStringUntil('\r');
    }
    // Serial.print(line);
    // Serial.println();
    // Serial.println("closing connection");

    // Update the last weather update time
    lastWeatherUpdateTime = currentTime;
  }
  return line;
}

// Вывод погоды на дисплей
void displayWeather()
{
  // Парсим JSON-данные
  DynamicJsonDocument doc(2000);                           // Создаем JSON-документ
  DeserializationError error = deserializeJson(doc, line); // Десериализуем JSON
  if (error)
  {
    if (millis() > 9000 && workingACP == 0)
    {
      // Serial.println("Failed to parse JSON");
      display1.setFont(u8g2_font_unifont_t_symbols);
      display1.drawUTF8(64, 42, "\u00B0"); // Добавляем символ градуса "°"
      display1.setFont(u8g2_font_ncenB08_tr);
      display1.drawStr(70, 42, "C"); // Добавляем букву "C" рядом с градусом
      // Serial.println("Failed to get temperature data");
    }
    return;
  }

  // Проверяем наличие данных о температуре
  bool hasTemperature = doc["main"]["temp"] != nullptr;

  if (millis() > 9000 && workingACP == 0)
  {
    if (hasTemperature)
    {
      // Получаем данные о погоде
      float temperature = doc["main"]["temp"]; // Температура
      String tempStr = "/ " + String(temperature - 273.15, 1);
      int tempWidth = display1.getStrWidth(tempStr.c_str());
      display1.setFont(u8g2_font_ncenB08_tr);
      display1.drawStr(64, 42, tempStr.c_str()); // Температура
      display1.setFont(u8g2_font_unifont_t_symbols);
      display1.drawUTF8(64 + tempWidth + 1, 42, "\u00B0"); // Добавляем символ градуса "°"
      display1.setFont(u8g2_font_ncenB08_tr);
      display1.drawStr(64 + tempWidth + 6, 42, "C"); // Добавляем букву "C" рядом с градусом
      // Serial.println("Temperature: " + String(temperature - 273.15, 1) + "°C");
    }
  }
}

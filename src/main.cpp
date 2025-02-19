#include <Arduino.h>
#include <Wire.h>
#include <U8g2lib.h>
// #include <U8x8lib.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include <EEPROM.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <RtcDS1302.h>
// #include <AHT10.h>
#include <iarduino_Pressure_BMP.h>
#include <UniversalTelegramBot.h>
#include <ButtonManager.h>
#include <Adafruit_AHTX0.h>
// #include <ScioSense_ENS160.h>
#include <DFRobot_ENS160.h>
#include <Adafruit_BusIO_Register.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_ADS1X15.h>
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
bool showWeather = false; // Флаг для отображения погоды

// Объявление функций
void netstatus();            // Функция отрисовки статуса инетернета
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
void showdetailedWeather();
void sendCommand(String command);
void esn160_aht21();
void IRAM_ATTR confirmButtonPressed();
void handleAction1();
void handleAction2();
void handleAction3();

// Объявление дисплеев и датчиков
U8G2_SSD1306_128X64_NONAME_F_HW_I2C display1(U8G2_R0, /* reset=*/U8X8_PIN_NONE);    // Дисплей 128x64
U8G2_SSD1306_128X32_UNIVISION_F_HW_I2C display2(U8G2_R0, /* reset=*/U8X8_PIN_NONE); // Дисплей 128x32
iarduino_Pressure_BMP bmp;                                                          // Датчик давления
WiFiClientSecure secured_client;
UniversalTelegramBot bot(BOT_TOKEN, secured_client); // Бот для отправки сообщений в Telegram
Adafruit_AHTX0 aht21;
Adafruit_ADS1115 ads;                   // Создаем объект для работы с ADS1115
DFRobot_ENS160_I2C ens160(&Wire, 0x52); // Указываем адрес I2C (0x53)

// Габариты дисплеев
#define DISPLAY1_WIDTH 128
#define DISPLAY1_HEIGHT 64
#define DISPLAY2_WIDTH 128
#define DISPLAY2_HEIGHT 32
#define transistorPin D5  // Пин для управления транзистором
#define CONFIRM_BUTTON D4 // Прерывание на цифровом пине
#define ANALOG_PIN A0     // Аналоговый вход
#define BUZZER_PIN D3     // Пин для подключения пищалки

const int BTN1_ADC = 1020;
const int BTN2_ADC = 630;
const int BTN3_ADC = 330;
const int TOLERANCE = 100; // Увеличил допуск

// Флаги событий кнопок
volatile bool isButtonPressed[3] = {false, false, false};

volatile int lastButton = -1;
volatile unsigned long lastPressTime = 0;
const unsigned long doublePressThreshold = 300; // Время для двойного нажатия (мс)

// Прерывание: фиксируем выбор кнопки
void IRAM_ATTR confirmButtonPressed()
{
  int adcValue = analogRead(ANALOG_PIN);

  if (abs(adcValue - BTN1_ADC) <= TOLERANCE)
  {
    isButtonPressed[0] = true;
  }
  else if (abs(adcValue - BTN2_ADC) <= TOLERANCE)
  {
    isButtonPressed[1] = true;
  }
  else if (abs(adcValue - BTN3_ADC) <= TOLERANCE)
  {
    isButtonPressed[2] = true;
  }
}
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
const String serverUrl = "http://192.168.0.10:8000/command"; // Замените на IP вашего ПК
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
const String allowed_chat_ids[] = {CHAT_ID, CHAT_ID_1};
const int num_allowed_chat_ids = sizeof(allowed_chat_ids) / sizeof(allowed_chat_ids[0]);

// Функция старта бота
void startTelegramBot()
{

  static bool start = false;
  if (start == true)
  {
    return;
  }
  else if (WiFi.status() == WL_CONNECTED && start == false && millis() > 12000)
  {
    Serial.print("Starting bot: ");
    Serial.println(millis());
    // Установка времени для TLS
    configTime(0, 0, "pool.ntp.org");
    secured_client.setTrustAnchors(&cert);
    // bot.setMyCommands(F("[]"));
    delay(1000);
    bot.sendMessage(allowed_chat_ids[0], "Bot is online and ready! Press /start to continue", "");
    // Устанавливаем команды бота
    const String commands = F("["
                              "{\"command\":\"RoomStatus\", \"description\":\"Room temp and hum\"},"
                              "{\"command\":\"test1\", \"description\":\"Send Pc sleep\"},"
                              "{\"command\":\"test2\", \"description\":\"Turn on/off\"},"
                              "{\"command\":\"test3\", \"description\":\"Get humidity\"}"
                              "]");
    bot.setMyCommands(commands); // Устанавливаем команды для бота
    Serial.print("Bot started at: ");
    Serial.println(millis());
    start = true;
  }
}

bool isChatIdAllowed(const String &chat_id)
{
  for (int i = 0; i < num_allowed_chat_ids; i++)
  {
    if (allowed_chat_ids[i] == chat_id)
    {
      return true;
    }
  }
  return false;
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
    else if (text == "/RoomStatus")
    {
      sensors_event_t humidity, temp;
      aht21.getEvent(&humidity, &temp);
      float temperature = temp.temperature;             // Read temperature from AHT21 sensor
      float humidityValue = humidity.relative_humidity; // Read humidity from AHT21 sensor
      String roomStatus = "Room temperature: " + String(temperature, 1) + "°C\n";
      roomStatus += "Room humidity: " + String(humidityValue, 1) + "%\n";
      bot.sendMessage(chat_id, roomStatus, "");
    }
    else if (text == "Пинг" || text == "пинг" || text == "ПИНГ" || text == "/пинг" || text == "/Пинг" || text == "/ПИНГ" || text == "пінг" || text == "Пінг" || text == "ПІНГ" || text == "/пінг" || text == "/Пінг" || text == "/ПІНГ")
    {
      bot.sendMessage(chat_id, "Понг", "");
    }
    else if (text == "Ping" || text == "ping" || text == "PING" || text == "/ping" || text == "/Ping" || text == "/PING")
    {
      bot.sendMessage(chat_id, "Pong", "");
    }
    else if (text == "Pong" || text == "pong" || text == "PONG" || text == "/pong" || text == "/Pong" || text == "/PONG")
    {
      bot.sendMessage(chat_id, "Ping", "");
    }
    else if (text == "Понг" || text == "понг" || text == "ПОНГ" || text == "/понг" || text == "/Понг" || text == "/ПОНГ")
    {
      bot.sendMessage(chat_id, "Пинг", "");
    }
    else if (text == "/test1" || text == "/test2" || text == "/test3")
    {
      if (!isChatIdAllowed(chat_id))
      {
        bot.sendMessage(chat_id, "You are not authorized to use this command.", "");
        continue;
      }

      if (text == "/test1")
      {
        bot.sendMessage(chat_id, "Just test1", "");
        sendCommand("notepad");
      }
      else if (text == "/test2")
      {
        bot.sendMessage(chat_id, "Just test2", "");
        sendCommand("mute_mic");
      }
      else if (text == "/test3")
      {
        bot.sendMessage(chat_id, "Just test3", "");
      }
    }
    else
    {
      bot.sendMessage(chat_id, "Invalid command. Please use /start to see the list of available commands.", "");
    }
  }
}
// Стартовая функция
void setup()
{
  pinMode(transistorPin, OUTPUT);   // Настроим пин как выход
  digitalWrite(transistorPin, LOW); // Включаем транзистор (LOW замыкает транзистор)
  pinMode(CONFIRM_BUTTON, INPUT_PULLUP);
  pinMode(BUZZER_PIN, OUTPUT);
  attachInterrupt(digitalPinToInterrupt(CONFIRM_BUTTON), confirmButtonPressed, FALLING);

  int failedAttemps = 0;
  Serial.begin(9600);
  EEPROM.begin(512);
  rtc.Begin();
  Wire.setClock(50000); // Установка I2C на 100 кГц

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
  display2.setI2CAddress(0x3C * 2); // Для 128x64

  // Инициализируем первый дисплей
  if (!display1.begin())
  {
    Serial.println("Не удалось инициализировать Дисплей 1");
    while (1)
      ;
  }
  else
  {
    Serial.println("Дисплей 1 инициализирован.");
  }
  display1.setDrawColor(1); // Установка белого цвета
  display1.clearBuffer();

  // Потом второй дисплей
  if (!display2.begin())
  {
    Serial.println("Не удалось инициализировать Дисплей 2");
    while (1)
      ;
  }
  else
  {
    Serial.println("Дисплей 2 инициализирован.");
  }
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
  // Инициализация AHT21
  if (!aht21.begin())
  {
    Serial.println("Не удалось инициализировать датчик AHT21!");
    while (1)
      ; // Ожидаем, пока не будет подключен
  }
  Serial.println("Датчик AHT21 инициализирован.");

  // Инициализация датчика BMP
  if (!bmp.begin())
  {
    Serial.println("Не удалось инициализировать датчик BMP!");
  }
  else
  {
    Serial.println("Датчик BMP инициализирован.");
  }

  // Инициализация модуля ADS1115
  if (!ads.begin())
  {
    Serial.println("Не удалось найти ADS1115. Проверьте соединения!");
    while (1)
      ;
  }
  Serial.println("ADS1115 успешно инициализирован.");

  delay(100);

  if (!ens160.begin())
  {
    Serial.println("Не вдалося ініціалізувати ENS160. Перевірте підключення.");
    while (1)
      ;
  }
  Serial.println("Датчик ENS160 инициализирован.");
  ens160.setPWRMode(ENS160_STANDARD_MODE); // Убедитесь, что датчик включен в нормальный режим работы

  display1.sendBuffer();
  display2.sendBuffer();
}
void loop()
{
  unsigned long currentTime = millis();
  static unsigned long lastUpdateTime = 0;

  handleAction1();
  handleAction2();
  handleAction3();

  if (currentTime - lastUpdateTime > 500)
  {
    lastUpdateTime = currentTime;
    startTelegramBot();
    display1.clearBuffer();
    display2.clearBuffer();
    if (workingACP == 1 && WiFi.status() != WL_CONNECTED)
    {
      accessPoint();
    }
    netstatus();
    updateClock();
    brightnessControl();
    tempandhum();
    weather();
    esn160_aht21(); // Функция получения данных
    displayWeather();
    int numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    while (numNewMessages)
    {
      handleNewMessages(numNewMessages);
      numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    }
    server.handleClient();
    showdetailedWeather();
    display1.sendBuffer();
    display2.sendBuffer();
  }
}

void netstatus()
{
  display1.setFont(u8g2_font_ncenB08_tr);

  if (WiFi.status() == WL_CONNECTED && millis() < 8800)
  {
    if (WiFi.status() == WL_CONNECTED)
    {
      display1.drawStr(0, 14, "Connected to WiFi");
      display1.drawStr(0, 28, WiFi.localIP().toString().c_str());
    }
    else
    {
      display1.drawStr(16, 14, "Failed to connect");
    }
  }
  if (millis() > 9000)
  {
    display1.setFont(u8g2_font_unifont_t_symbols);
    if (WiFi.status() == WL_CONNECTED && workingACP == 0 && showWeather == 0)
    {
      display1.drawUTF8(98, 24, "\u2714"); // Использование символа галочки
    }
    if (WiFi.status() != WL_CONNECTED && workingACP == 0 && showWeather == 0)
    {
      display1.drawUTF8(98, 24, "\u2718"); // Использование символа крестика
    }
  }
}

void accessPoint()
{
  static bool accessPointInitialized = false;
  static unsigned long lastClientHandle = 0;
  static unsigned long lastDisplayUpdate = 0;

  if (workingACP == 0 || WiFi.status() == WL_CONNECTED)
  {
    if (accessPointInitialized)
    {
      WiFi.softAPdisconnect(true);
      accessPointInitialized = false;
      Serial.println("Access Point stopped");
    }
    return;
  }

  if (!accessPointInitialized)
  {
    WiFi.disconnect();
    WiFi.mode(WIFI_AP);

    unsigned long startTime = millis();
    WiFi.softAPConfig(local_IP, gateway, subnet);
    Serial.printf("softAPConfig took: %lu ms\n", millis() - startTime);

    startTime = millis();
    WiFi.softAP(ap_ssid, ap_password);
    Serial.printf("softAP took: %lu ms\n", millis() - startTime);

    server.on("/", handleRoot);
    server.on("/submit", HTTP_POST, handleSubmit);
    server.begin();
    accessPointInitialized = true;

    Serial.println("Access Point started");
    Serial.println("IP Address:");
    Serial.println(WiFi.softAPIP());
  }

  if (millis() - lastClientHandle > 50)
  {
    server.handleClient();
    lastClientHandle = millis();
  }

  if (millis() - lastDisplayUpdate > 100)
  {
    display1.clearBuffer();
    display2.clearBuffer();

    display1.setFont(u8g2_font_ncenB08_tr);
    display1.drawStr(8, 12, "Access Point started");
    display1.setFont(u8g2_font_ncenB12_tr);
    display1.drawStr(4, 28, WiFi.softAPIP().toString().c_str());

    static int textPosX1 = 12;
    const char *scrollText = "Press any button to continue";
    int textWidth = display1.getStrWidth(scrollText);
    int spacing = 64;

    textPosX1 -= 4;
    if (textPosX1 < -textWidth)
    {
      textPosX1 += textWidth - spacing;
    }

    display1.setFont(u8g2_font_ncenB08_tr);
    display1.drawStr(textPosX1, 48, scrollText);
    display1.drawStr(textPosX1 + textWidth - spacing, 48, scrollText);

    display2.setFont(u8g2_font_ncenB08_tr);
    display2.drawStr(0, 10, "Connect to WiFi");
    display2.drawStr(0, 20, ("SSID: " + String(ap_ssid)).c_str());
    display2.drawStr(0, 30, ("Password: " + String(ap_password)).c_str());

    display1.sendBuffer();
    display2.sendBuffer();
    lastDisplayUpdate = millis();
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
    snprintf(buffer, sizeof(buffer), "%02d:%02d", t.Hour(), t.Minute());
    // display2.clearBuffer();                                  // Очищаем экран перед выводом
    display2.drawStr((DISPLAY2_WIDTH / 2) - 14, 12, buffer); // Время по центру экрана

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
    DynamicJsonDocument doc(512);      // Создаем JSON-документ
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

#define NUM_SAMPLES 10
float VoutBuffer[NUM_SAMPLES] = {0};
int sampleIndex = 0;

void brightnessControl()
{
  int16_t analogValue = ads.readADC_SingleEnded(0); // Канал 0
  const float R2 = 10000.0;                         // Резистор 10 кОм
  const float Vin = 3.3;                            // Напряжение питания 3.3 В

  // Преобразование в напряжение
  float Vout = analogValue * (Vin / 32768.0); // Для ADS1115 (±6.144 В диапазон)
  if (Vout <= 0)
    Vout = 0.01; // Предотвращаем деление на ноль

  // Расчёт сопротивления
  float R1 = (R2 * (Vin - Vout)) / Vout;

  // Вывод данных для отладки
  // Serial.print("Vout: ");
  // Serial.println(Vout);
  // Serial.print("R1: ");
  // Serial.println(R1);

  // Масштабируем яркость
  // Первый дисплей (для большого диапазона сопротивлений)
  int brightness1 = map(R1, 50000, 600000, 255, 50);
  brightness1 = constrain(brightness1, 50, 255); // Ограничиваем диапазон

  // Второй дисплей (для более узкого диапазона)
  int brightness2 = map(R1, 50000, 300000, 255, 0);
  brightness2 = constrain(brightness2, 0, 255); // Ограничиваем диапазон

  // Вывод для отладки
  // Serial.print("Brightness1: ");
  // Serial.println(brightness1);
  // Serial.print("Brightness2: ");
  // Serial.println(brightness2);

  // Применяем яркость
  display1.setContrast(brightness1);
  display2.setContrast(brightness2);
}

// Получение температуры и влажности
void tempandhum()
{
  sensors_event_t humidityEvent, tempEvent;
  aht21.getEvent(&humidityEvent, &tempEvent);
  float temperature = tempEvent.temperature;        // Read temperature from AHT21 sensor
  float humidity = humidityEvent.relative_humidity; // Read humidity from AHT21 sensor
  float pressure = bmp.pressure;
  temperature = bmp.temperature;
  if (millis() > 9000 && workingACP == 0 && showWeather == 0)
  {
    // Prepare strings for display
    char tempStr[10];
    char humStr[10];
    char pressureStr[10];
    snprintf(tempStr, sizeof(tempStr), "%.1f", temperature); // Convert temperature to string
    snprintf(humStr, sizeof(humStr), "%.1f", humidity);
    snprintf(pressureStr, sizeof(pressureStr), "%.1f", pressure); // Convert pressure to hPa

    // Display temperature, humidity, and pressure with labels
    display1.setFont(u8g2_font_ncenB08_tr);

    // Display pressure
    display1.drawStr(0, 10, "Pres:");
    int pressureWidth = display1.getStrWidth(pressureStr); // Вычисляем ширину строки с давлением
    display1.drawStr(40, 10, pressureStr);                 // Давление
    display1.drawStr(44 + pressureWidth + 1, 10, "mmHg");  // Добавляем единицу измерения давления "hPa"

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
    Serial.println("Get weather data failed"); // если ошибка, сообщаем об этом
    return;                                    // и запускаем заного
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

void showdetailedWeather()
{
  static unsigned long startMillis = 0; // Начальное время
  const unsigned long interval = 10000; // Интервал в миллисекундах (10 секунд)
  static bool parsed = false;           // Флаг парсинга
  static String weather;                // Данные погоды
  static float windSpeed = 0, humidity = 0, pressure_mmHg = 0;

  if (showWeather == 1)
  {
    // Парсинг JSON только один раз при начале отображения
    if (!parsed)
    {
      static DynamicJsonDocument doc(512);
      DeserializationError error = deserializeJson(doc, line);
      if (error)
      {
        Serial.println("Ошибка разбора JSON");
        return;
      }

      // Извлечение данных о погоде
      weather = doc["weather"][0]["description"].as<String>();
      windSpeed = doc["wind"]["speed"].as<float>(); // Получение скорости ветра
      humidity = doc["main"]["humidity"].as<float>();
      float pressure_hPa = doc["main"]["pressure"].as<float>();
      pressure_mmHg = pressure_hPa * 0.75006; // Преобразование давления в мм рт. ст.

      startMillis = millis(); // Запуск таймера
      parsed = true;          // Флаг парсинга установлен
    }

    // Отображение данных на дисплее
    display1.clearBuffer();
    display1.setFont(u8g2_font_ncenB08_tr);
    display1.drawStr(0, 10, "Weather:");
    display1.drawStr(0, 20, weather.c_str());
    display1.drawStr(0, 30, ("Wind: " + String(windSpeed, 1) + " m/s").c_str());
    display1.drawStr(0, 40, ("Hum: " + String(humidity, 1) + " %").c_str());
    display1.drawStr(0, 50, ("Press: " + String(pressure_mmHg, 1) + " mmHg").c_str());
    display1.sendBuffer();

    // Проверка окончания интервала
    if (millis() - startMillis >= interval)
    {
      Serial.println("Таймер завершён!");
      showWeather = 0; // Отключение показа
      parsed = false;  // Сброс для следующего отображения
    }
  }
}

// Вывод погоды на дисплей
void displayWeather()
{
  unsigned long start = millis();

  // Парсим JSON-данные
  DynamicJsonDocument doc(512); // Уменьшен размер документа
  DeserializationError error = deserializeJson(doc, line);

  if (error)
  {
    if (millis() > 9000 && workingACP == 0 && showWeather == 0)
    {
      display1.setFont(u8g2_font_unifont_t_symbols);
      display1.drawUTF8(64, 42, "\u00B0"); // Градус
      display1.setFont(u8g2_font_ncenB08_tr);
      display1.drawStr(70, 42, "C"); // Буква "C"
    }
    // Serial.println("Failed to parse JSON");
    return;
  }
  // Serial.printf("JSON parsing took: %lu ms\n", millis() - start);

  // Проверяем наличие данных о температуре
  bool hasTemperature = doc["main"]["temp"] != nullptr;

  if (millis() > 9000 && workingACP == 0 && hasTemperature && showWeather == 0)
  {
    // Получаем данные о погоде
    float temperature = doc["main"]["temp"]; // Температура
    String weather = doc["weather"][0]["description"];
    String tempStr = "/ " + String(temperature - 273.15, 1) + "\u00B0C"; // Формируем строку
                                                                         // Формируем строку
    display1.setFont(u8g2_font_ncenB08_tr);
    display1.drawStr(64, 42, tempStr.c_str());
    display1.setFont(u8g2_font_ncenB10_tr); // Увеличиваем шрифт
    int weatherWidth = display1.getStrWidth(weather.c_str());
    int weatherX = (DISPLAY1_WIDTH - weatherWidth) / 2;
    display1.drawStr(weatherX, 58, weather.c_str()); // Выводим текст по центру экрана
    // Serial.printf("Display update took: %lu ms\n", millis() - start);
  }
}

void sendCommand(String command)
{
  HTTPClient http;
  WiFiClient client;
  http.begin(client, serverUrl);
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");

  // Формируем данные для отправки
  String postData = "command=" + command;
  int httpResponseCode = http.POST(postData);

  if (httpResponseCode > 0)
  {
    Serial.printf("Response code: %d\n", httpResponseCode);
    String response = http.getString();
    Serial.println(response);
  }
  else
  {
    Serial.printf("Error on sending POST: %s\n", http.errorToString(httpResponseCode).c_str());
  }

  http.end();
}

void esn160_aht21()
{
  // Получаем данные с AHT21
  sensors_event_t humidity, temp;
  aht21.getEvent(&humidity, &temp);

  // Выводим данные с AHT21
  // Serial.println(F("Данные с AHT21:"));
  // Serial.print(F("Температура: "));
  // Serial.print(temp.temperature);
  // Serial.println(F(" °C"));

  // Serial.print(F("Влажность: "));
  // Serial.print(humidity.relative_humidity);
  // Serial.println(F(" %"));

  // Устанавливаем мощность в обычный режим для работы с данными
  ens160.setPWRMode(ENS160_STANDARD_MODE);

  // Устанавливаем данные о температуре и влажности для компенсации
  ens160.setTempAndHum(temp.temperature, humidity.relative_humidity);

  // Получаем CO2 эквивалент и VOC
  uint16_t eco2 = ens160.getECO2(); // CO2 эквивалент
  uint16_t tvoc = ens160.getTVOC(); // TVOC эквивалент

  // Выводим данные с ENS160
  // Serial.println(F("Данные с ENS160:"));
  // Serial.print(F("CO2 эквивалент: "));
  // Serial.print(eco2);
  // Serial.println(F(" ppm"));

  // Serial.print(F("VOC эквивалент: "));
  // Serial.print(tvoc);
  // Serial.println(F(" ppb"));

  // Serial.println(F("-----------------------------"));
}

void handleAction1()
{
  if ((isButtonPressed[0]) && WiFi.status() != WL_CONNECTED && workingACP == 1)
  {
    workingACP = 0;
    isButtonPressed[0] = false;
  }
  if (isButtonPressed[0] && workingACP == 0)
  {
    Serial.println("Кнопка 1:Нажата");
    showWeather = 1;
    showdetailedWeather();
    tone(BUZZER_PIN, 1000, 300);

    isButtonPressed[0] = false;
  }
}

void handleAction2()
{
  const unsigned long pressDuration = 1000; // Задержка 1 секунда для включения транзистора
  if ((isButtonPressed[1]) && WiFi.status() != WL_CONNECTED && workingACP == 1)
  {
    workingACP = 0;
    isButtonPressed[1] = false;
  }
  if (isButtonPressed[1] && workingACP == 0)
  {
    Serial.println("Кнопка 2:Нажата");
    digitalWrite(transistorPin, HIGH);
    delay(pressDuration);
    digitalWrite(transistorPin, LOW);
    tone(BUZZER_PIN, 500, 150);
    delay(50);
    tone(BUZZER_PIN, 500, 150);
    isButtonPressed[1] = false;
  }
}

void handleAction3()
{
  if ((isButtonPressed[2]) && WiFi.status() != WL_CONNECTED && workingACP == 1)
  {
    workingACP = 0;
    isButtonPressed[2] = false;
  }
  if (isButtonPressed[2] && workingACP == 0)
  {
    Serial.println("Кнопка 3:Нажата");
    tone(BUZZER_PIN, 200, 100);
    delay(50);
    tone(BUZZER_PIN, 200, 100);
    delay(50);
    tone(BUZZER_PIN, 200, 100);
    isButtonPressed[2] = false;
  }
}

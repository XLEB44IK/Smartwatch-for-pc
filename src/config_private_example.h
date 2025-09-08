// config_private_example.h — пример приватной конфигурации
// Скопируйте этот файл в src/config_private.h и заполните реальные значения.

#ifndef CONFIG_PRIVATE_EXAMPLE_H
#define CONFIG_PRIVATE_EXAMPLE_H

// Ключ API OpenWeatherMap
const char *WeatherApiKey = "YOUR_OPENWEATHERMAP_API_KEY";

// id города (по OpenWeatherMap) или координаты
const char *Cityid = "YOUR_CITY_ID";
const char *CityLat = "47.8167";
const char *CityLon = "35.1833";

// Telegram bot token и разрешённые chat_id
const char *BOT_TOKEN = "YOUR_TELEGRAM_BOT_TOKEN";
const char *CHAT_ID = "YOUR_CHAT_ID";
const char *CHAT_ID_1 = "OPTIONAL_CHAT_ID_2";

#endif

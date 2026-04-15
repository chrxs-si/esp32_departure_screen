#include <HTTPClient.h>

struct WeatherData {
  int temperature_2m;
  int   weather_code;
  float rain;
  float showers;
  float snowfall;
  int   cloud_cover;
  bool  is_day;
};

String fetchWeatherJson(float latitude, float longitude);

bool parseWeatherJson(const String &json, WeatherData &data);
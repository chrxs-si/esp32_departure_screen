#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include "weather_data.h"

String fetchWeatherJson(float latitude, float longitude) {
  if (WiFi.status() != WL_CONNECTED) {
    return "";
  }

  String url = "https://api.open-meteo.com/v1/forecast?";
  url += "latitude=" + String(latitude, 6);
  url += "&longitude=" + String(longitude, 6);
  url += "&current=temperature_2m,weather_code,rain,showers,snowfall,cloud_cover,is_day";
  url += "&timezone=Europe%2FBerlin";
  url += "&forecast_days=1";

  HTTPClient http;
  http.begin(url);

  int httpCode = http.GET();
  String payload = "";

  if (httpCode == HTTP_CODE_OK) {
    payload = http.getString();
  }

  http.end();
  return payload;
}

bool parseWeatherJson(const String &json, WeatherData &data) {
  if (json.length() == 0) return false;

  StaticJsonDocument<1024> doc;
  DeserializationError error = deserializeJson(doc, json);
  if (error) return false;

  JsonObject current = doc["current"];
  if (current.isNull()) return false;

  data.temperature_2m = (int)round(current["temperature_2m"].as<float>());
  data.weather_code   = current["weather_code"].as<int>();
  data.rain           = current["rain"].as<float>();
  data.showers        = current["showers"].as<float>();
  data.snowfall       = current["snowfall"].as<float>();
  data.cloud_cover    = current["cloud_cover"].as<int>();
  data.is_day         = current["is_day"].as<int>() == 1;

  return true;
}
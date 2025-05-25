// ESP8266 Weather Station with OpenWeatherMap + DHT Sensor
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include <DHT.h>
#include <ArduinoJson.h>

// WiFi Credentials
const char* ssid = "dhruv";
const char* password = "7892686064";

// OpenWeatherMap API Key
String apiKey = "ac04718d80c558277bd1838c52f865ed";

// DHT Sensor Settings
#define DHTPIN D2
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

ESP8266WebServer server(80);

String lastWeatherCard = "";
String cityCard = "<div class='rounded-xl p-3 text-white font-semibold text-sm bg-gradient-to-b from-[#4db8ff] to-[#4a5cff] flex flex-col justify-center items-center mt-4'><span class='text-xl'>--°C</span><span class='font-bold'>--</span><span class='text-xs mt-1'>---</span></div>";

String htmlPage(String weatherCard, String dhtCard) {
  return "<!DOCTYPE html><html lang='en'><head><meta charset='utf-8'/>"
  "<meta name='viewport' content='width=device-width, initial-scale=1'/>"
  "<title>ESP8266 Weather Station</title>"
  "<script src='https://cdn.tailwindcss.com'></script>"
  "<link href='https://fonts.googleapis.com/css2?family=Playfair+Display&display=swap' rel='stylesheet'/>"
  "<style>body { font-family: 'Playfair Display', serif; }</style></head>"
  "<body class='bg-[#b7d1ff] min-h-screen flex items-center justify-center p-4'>"
  "<div class='max-w-sm w-full space-y-4'>"
  "<h1 class='text-3xl text-center font-black'>ESP8266 Weather Station</h1>"
  "<form class='flex justify-center' action='/weather'>"
  "<input name='city' class='text-sm rounded border border-gray-400 px-2 py-1 w-48' placeholder='Enter your city' type='text'/>"
  "<button type='submit' class='ml-2 bg-blue-500 text-white px-3 py-1 rounded'>Search</button>"
  "</form>" + weatherCard + dhtCard + "</div></body></html>";
}

String generateWeatherCard(String city, String temp, String desc, String icon) {
  String iconUrl = "http://openweathermap.org/img/wn/" + icon + "@2x.png";
  return "<div class='rounded-xl p-3 text-white font-semibold text-sm bg-gradient-to-b from-[#4db8ff] to-[#4a5cff] flex flex-col justify-center items-center mt-4'>"
         "<img src='" + iconUrl + "' class='w-16 h-16'/>"
         "<span class='text-xl'>" + temp + "°C</span>"
         "<span class='font-bold'>" + desc + "</span>"
         "<span class='text-xs mt-1'>" + city + "</span></div>";
}

String generateDHTCard(float temp) {
  return "<div class='rounded-xl p-3 text-white font-semibold text-sm bg-gradient-to-b from-[#4db8ff] to-[#4a5cff] flex flex-col justify-center items-center mt-4'>"
         "<i class='fas fa-thermometer-half text-white text-lg'></i>"
         "<span class='text-xl'>" + String(temp, 1) + "°C</span>"
         "<span class='font-bold'>Local Temp</span>"
         "<span class='text-xs mt-1'>From DHT11 Sensor</span></div>";
}

void handleRoot() {
  float localTemp = dht.readTemperature();
  String dhtCard = generateDHTCard(localTemp);
  server.send(200, "text/html", htmlPage(cityCard, dhtCard));
}

void handleWeather() {
  if (!server.hasArg("city")) {
    server.send(400, "text/plain", "City not provided");
    return;
  }

  String city = server.arg("city");
  String apiUrl = "http://api.openweathermap.org/data/2.5/weather?q=" + city + "&appid=" + apiKey + "&units=metric";

  WiFiClient client;
  HTTPClient http;
  http.begin(client, apiUrl);
  int httpCode = http.GET();

  float localTemp = dht.readTemperature();
  String dhtCard = generateDHTCard(localTemp);

  if (httpCode == 200) {
    String payload = http.getString();
    StaticJsonDocument<1024> doc;
    deserializeJson(doc, payload);

    String temp = String(doc["main"]["temp"].as<float>(), 1);
    String desc = doc["weather"][0]["description"].as<String>();
    String icon = doc["weather"][0]["icon"].as<String>();
    String cityName = doc["name"].as<String>();

    lastWeatherCard = generateWeatherCard(cityName, temp, desc, icon);
    server.send(200, "text/html", htmlPage(lastWeatherCard, dhtCard));
  } else {
    server.send(200, "text/html", htmlPage("<p class='text-center text-red-500 mt-4'>City not found or API error!</p>", dhtCard));
  }
  http.end();
}

void setup() {
  Serial.begin(115200);
  dht.begin();
  WiFi.begin(ssid, password);
  Serial.print("Connecting to Wi-Fi");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nConnected! IP address: " + WiFi.localIP().toString());

  server.on("/", handleRoot);
  server.on("/weather", handleWeather);
  server.begin();
  Serial.println("Web server started.");
}

void loop() {
  server.handleClient();
}

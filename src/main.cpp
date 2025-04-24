#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

const char* ssid = "Wokwi-GUEST";
const char* password = "";
String server = "https://api.open-meteo.com/v1/forecast?latitude=-7.9797&longitude=112.6304&current_weather=true&timezone=Asia%2FJakarta";

LiquidCrystal_I2C lcd(0x27, 16, 2);

const int buttonNext = 2;
const int buttonPrev = 15;

int currentPage = 0;
const int totalPages = 2;

String temperature = "N/A";
String windspeed = "N/A";
String winddirection = "N/A";

unsigned long lastButtonTimeNext = 0;
unsigned long lastButtonTimePrev = 0;
const unsigned long debounceDelay = 200;

unsigned long lastRefreshTime = 0;
const unsigned long refreshInterval = 3000;

unsigned long lastWeatherFetchTime = 0;
const unsigned long weatherFetchInterval = 60000;

void fetchWeather();
void handleButtons();
void displayPage();

void setup() {
    Serial.begin(115200);
    lcd.init();
    lcd.backlight();
    lcd.setCursor(0, 0);
    lcd.print("Weather Info:");

    pinMode(buttonNext, INPUT_PULLUP);
    pinMode(buttonPrev, INPUT_PULLUP);

    WiFi.begin(ssid, password);

    lcd.setCursor(0, 1);
    lcd.print("Connecting...");
    
    unsigned long startTime = millis();
    while (WiFi.status() != WL_CONNECTED) {
        if (millis() - startTime >= 10000) {
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("WiFi Failed");
            Serial.println("WiFi connection failed");
            return;
        }
        delay(1000);
        Serial.println("Connecting to WiFi...");
    }

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Connected!");
    delay(2000);
    lcd.clear();
    
    fetchWeather();
    displayPage();
}

void loop() {
    handleButtons();

    unsigned long currentTime = millis();
    if (currentTime - lastWeatherFetchTime >= weatherFetchInterval) {
        fetchWeather();
        lastWeatherFetchTime = currentTime;
    }

    if (currentTime - lastRefreshTime >= refreshInterval) {
        displayPage();
        lastRefreshTime = currentTime;
    }

    delay(50);
}

void fetchWeather() {
    if (WiFi.status() == WL_CONNECTED) {
        HTTPClient http;
        http.begin(server);
        int httpCode = http.GET();

        if (httpCode > 0) {
            String payload = http.getString();
            Serial.println(payload);

            StaticJsonDocument<1024> doc;
            DeserializationError error = deserializeJson(doc, payload);

            if (!error) {
                float temp = doc["current_weather"]["temperature"];
                float wind = doc["current_weather"]["windspeed"];
                int windDir = doc["current_weather"]["winddirection"];
                temperature = String(temp, 1);
                windspeed = String(wind, 1);
                winddirection = String(windDir);
                
                Serial.println("Parsed values from JSON:");
                Serial.println("Temperature: " + temperature + "°C");
                Serial.println("Wind Speed: " + windspeed + " km/h");
                Serial.println("Wind Direction: " + winddirection + "°");
            } else {
                Serial.println("Failed to parse JSON");
                temperature = "Error";
                windspeed = "Error";
                winddirection = "Error";
            }
        } else {
            Serial.println("HTTP request failed");
            temperature = "Error";
            windspeed = "Error";
            winddirection = "Error";
        }

        http.end();
    }
}

void handleButtons() {
    unsigned long now = millis();

    int nextState = digitalRead(buttonNext);
    if (nextState == LOW && now - lastButtonTimeNext > debounceDelay) {
        currentPage = (currentPage + 1) % totalPages;
        lastButtonTimeNext = now;
        Serial.println("Button NEXT pressed - Page: " + String(currentPage + 1) + "/" + String(totalPages));
        displayPage();
        lastRefreshTime = now;
    }

    int prevState = digitalRead(buttonPrev);
    if (prevState == LOW && now - lastButtonTimePrev > debounceDelay) {
        currentPage = (currentPage - 1 + totalPages) % totalPages;
        lastButtonTimePrev = now;
        Serial.println("Button PREV pressed - Page: " + String(currentPage + 1) + "/" + String(totalPages));
        displayPage();
        lastRefreshTime = now;
    }
    
    static unsigned long lastDebugTime = 0;
    if (now - lastDebugTime > 1000) {
        Serial.print("Button states - Next: ");
        Serial.print(nextState == LOW ? "PRESSED" : "RELEASED");
        Serial.print(", Prev: ");
        Serial.println(prevState == LOW ? "PRESSED" : "RELEASED");
        lastDebugTime = now;
    }
}

void displayPage() {
    lcd.clear();
    
    lcd.setCursor(14, 0);
    lcd.print(currentPage + 1);
    lcd.print("/");
    lcd.print(totalPages);
    
    if (currentPage == 0) {
        lcd.setCursor(0, 0);
        lcd.print("Temp: ");
        lcd.print(temperature);
        lcd.print((char)223);
        lcd.print("C");
        
        lcd.setCursor(0, 1);
        lcd.print("Weather Info >>");
    } else if (currentPage == 1) {
        lcd.setCursor(0, 0);
        lcd.print("Wind: ");
        lcd.print(windspeed);
        lcd.print(" km/h");

        lcd.setCursor(0, 1);
        lcd.print("Dir: ");
        lcd.print(winddirection);
        lcd.print((char)248);
    }
}

#include <Arduino.h>
#include <ArduinoLog.h>
#include <WiFi.h>
#include <ESP_Google_Sheet_Client.h>
#include <LittleFS.h>
#include <iostream>
#include "GoogleTmpLogger.h"
#include <LiquidCrystal_I2C.h>

#define configFilePath "/config.json"

#define uS_TO_S_FACTOR 1000000ULL
#define TIME_TO_SLEEP 60 // seconds

#define SDA 14
#define SCL 13
#define LCD_ADDRESS 0x3F

String WIFI_SSID = "";
String WIFI_PASSWORD = "";

GoogleTmpLogger tmpLogger;
LiquidCrystal_I2C lcd(LCD_ADDRESS, 16, 2);

const int backlightButtonPin = 2;    // Pin connected to the button
int backlightButtonLastState = HIGH; // Variable to store the last button state
bool backlightOn = true;

void setup()
{
    Serial.begin(115200); // make sure your Serial Monitor is also set at this baud rate.
    Log.begin(LOG_LEVEL_VERBOSE, &Serial);

    Log.infoln("Setup started");

    setupLCD();

    FirebaseJson configJson;
    if (setSecretsFromConfig(configJson))
    {
        connectToWiFi();

        setupTime();

        tmpLogger.setup(configJson);
    }
    else
    {
        Log.error("Could not load secrets from config");
        while (1)
            ;
    }

    esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);

    Log.infoln("Setup complete");
}

void loop()
{
    TmpMeasurement measurement = tmpLogger.logTmpTask();

    lcd.setCursor(0, 0); // Move the cursor to row 1, column 0
    lcd.print("Tmp: ");
    lcd.print(measurement.temperature);
    lcd.print((char)223);
    lcd.print("C   ");

    //handleLCDBacklightButtonClick();

    //delay(10000);

    esp_deep_sleep_start();
}

void connectToWiFi()
{
    // WiFi.setAutoReconnect(true);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    Serial.print("Connecting to Wi-Fi");
    unsigned long ms = millis();
    while (WiFi.status() != WL_CONNECTED)
    {
        Serial.print(".");
        delay(300);
    }
    Serial.println();
    Serial.print("Connected with IP: ");
    Serial.println(WiFi.localIP());
    Serial.println();
}

void setupTime()
{
    char *ntpServer = "pool.ntp.org";
    long gmtOffset_sec = 2 * 3600;
    int daylightOffset_sec = 0;
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

    struct tm timeinfo;
    for (int i = 0; i < 10; i++)
    {
        if (getLocalTime(&timeinfo))
        {
            Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
            return;
        }
        Serial.println("Waiting for NTP sync...");
        delay(1000);
    }
    Serial.println("Failed to sync time.");
}

void setupLCD()
{
    Wire.begin(SDA, SCL); // attach the IIC pin
    if (!i2CAddrTest(LCD_ADDRESS))
    {
        Log.infoln("LCD not found at address 0x3F, trying 0x27");
        lcd = LiquidCrystal_I2C(0x27, 16, 2);
    }
    lcd.init();              // LCD driver initialization
    //lcd.backlight();         // Open the backlight
    lcd.setCursor(0, 0);     // Move the cursor to row 0, column 0
    lcd.print("LCD setup!"); // The print content is displayed on the LCD
}

void readConfig(FirebaseJson &json)
{
    String fileText = readFile(configFilePath);

    int fileSize = fileText.length();
    Serial.println("Config file size: " + fileSize);

    json.setJsonData(fileText);

    WIFI_SSID = getJsonValue(json, "wifi_ssid");
    WIFI_PASSWORD = getJsonValue(json, "wifi_password");

    Log.infoln("Config -> WiFi SSID: %s, WIFI_PASSWORD: %s", WIFI_SSID.c_str(), WIFI_PASSWORD.c_str());
}

bool setSecretsFromConfig(FirebaseJson &configJson)
{
    if (!LittleFS.begin(false))
    {
        Serial.println("LITTLEFS Mount failed");
        Serial.println("Did not find filesystem; starting format");
        // format if begin fails
        if (!LittleFS.begin(true))
        {
            Serial.println("LITTLEFS mount failed");
            Serial.println("Formatting not possible");
            return false;
        }
        else
        {
            Serial.println("Formatting");
        }
    }

    Serial.println("setup -> SPIFFS mounted successfully");
    // if (readConfig().serializedBufferLength() == 0)
    // {
    //     Serial.println("setup -> Could not read Config file");
    //     return false;
    // }

    readConfig(configJson);

    return true;
}

String readFile(String filename)
{
    File file = LittleFS.open(filename);
    if (!file)
    {
        Serial.println("Failed to open file for reading");
        return "";
    }

    String fileText = "";
    while (file.available())
    {
        fileText = file.readString();
    }
    file.close();
    return fileText;
}

String getJsonValue(FirebaseJson json, String key)
{
    FirebaseJsonData result;
    json.get(result, key);

    if (result.success)
    {
        return result.to<String>();
    }
    else
    {
        return "";
    }
}

bool i2CAddrTest(uint8_t addr)
{
    Wire.beginTransmission(addr);
    if (Wire.endTransmission() == 0)
    {
        return true;
    }
    return false;
}

void handleLCDBacklightButtonClick()
{
    int buttonState = digitalRead(backlightButtonPin); // Read the button state

    // Check if the button is pressed (LOW) and was previously not pressed
    if (buttonState == LOW && backlightButtonLastState == HIGH)
    {
        delay(50);                                  // Debounce delay
        if (digitalRead(backlightButtonPin) == LOW) // Confirm button press
        {
            // Toggle the backlight
            backlightOn = !backlightOn;
            if (backlightOn)
            {
                lcd.backlight();
            }
            else
            {
                lcd.noBacklight();
            }
        }
    }

    backlightButtonLastState = buttonState;
}
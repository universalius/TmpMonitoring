#include <Arduino.h>
#include <ArduinoLog.h>
#include <WiFi.h>
#include <ESP_Google_Sheet_Client.h>
#include <LittleFS.h>
#include <iostream>

#define thermistorPin 1
#define routerPowerOnPin 2

#define configFilePath "/config.json"

#define MaxMeasurementsCount 60

#define uS_TO_S_FACTOR 1000000ULL
#define TIME_TO_SLEEP 60 // seconds

struct TmpMeasurement
{
    double temperature;
    char *time;
};

String WIFI_SSID = "";
String WIFI_PASSWORD = "";

String GOOGLE_PROJECT_ID = "";

// Service Account's client email
String GOOGLE_CLIENT_EMAIL = "";

// Your email to share access to spreadsheet
const char *GOOGLE_USER_EMAIL = "";

String GOOGLE_API_PRIVATE_KEY = "";

bool logTemperatureTaskComplete = false;

String spreadsheetId = "1RQlHQZr8uJ4i6gAF6mfdCsI_RbmNrU4sZ5N9aiCkI9g";

RTC_DATA_ATTR char spreadsheetTab[50] = "";

RTC_DATA_ATTR tm spreadsheetTabCreated;

RTC_DATA_ATTR int lastRow = 0;

RTC_DATA_ATTR int measurementIndex = 0;

RTC_DATA_ATTR TmpMeasurement temperatures[100];

void setup()
{
    Serial.begin(115200); // make sure your Serial Monitor is also set at this baud rate.
    Log.begin(LOG_LEVEL_VERBOSE, &Serial);

    Log.infoln("Setup started");
    if (setSecretsFromConfig())
    {
        connectToWiFi();

        setupGoogleSheet();

        setupTime();
    }
    else
    {
        Log.error("Could not load secrets from config");
        while (1)
            ;
    }

    esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);

    Log.infoln("spreadsheetTab : %s, lastRow: %d, day: %d", spreadsheetTab, lastRow, spreadsheetTabCreated.tm_mday);

    Log.infoln("Setup complete");
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

void loop()
{

    // Call ready() repeatedly in loop for authentication checking and processing
    bool ready = GSheet.ready();
    if (ready)
    {
        if (measurementIndex >= MaxMeasurementsCount - 1)
        {
            digitalWrite(routerPowerOnPin, HIGH);
            delay(1000 * 30);

            createGoogleSheetTab();

            if (lastRow == 0)
            {
                readLastRow();
            }

            readAndRememberTemperature();
            logTemperature();

            digitalWrite(routerPowerOnPin, LOW);
            esp_deep_sleep_start();
        }
        else
        {
            readAndRememberTemperature();
            //esp_deep_sleep_start();
        }
    }

    delay(500);
}

TmpMeasurement readTemperature()
{
    tm now = getTimeNow();

    char timeString[20];
    strftime(timeString, sizeof(timeString), "%Y-%m-%d %H:%M:%S", &now);

    int adcValue = analogRead(thermistorPin);                       // read ADC pin
    double voltage = (float)adcValue / 4095.0 * 3.3;                // calculate voltage
    double Rt = 10 * voltage / (3.3 - voltage);                     // calculate resistance value of thermistor
    double tempK = 1 / (1 / (273.15 + 25) + log(Rt / 10) / 3950.0); // calculate temperature (Kelvin)
    double tempC = tempK - 273.15;                                  // calculate temperature (Celsius)
    Serial.printf("Pin value : %d,\tVoltage : %.2fV, \tTemperature : %.2fC\n", adcValue, voltage, tempC);

    return {tempC, timeString};
}

void addTemperatureToMemory(TmpMeasurement temperature)
{
    if (measurementIndex < 100)
    {
        temperatures[measurementIndex] = temperature;
        measurementIndex++;
    }
}

void readAndRememberTemperature()
{
    TmpMeasurement temperature = readTemperature();
    addTemperatureToMemory(temperature);
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

void tokenStatusCallback(TokenInfo info)
{
    if (info.status == esp_signer_token_status_error)
    {
        Serial.printf("Token info: type = %s, status = %s\n", GSheet.getTokenType(info).c_str(), GSheet.getTokenStatus(info).c_str());
        Serial.printf("Token error: %s\n", GSheet.getTokenError(info).c_str());
    }
    else
    {
        Serial.printf("Token info: type = %s, status = %s\n", GSheet.getTokenType(info).c_str(), GSheet.getTokenStatus(info).c_str());
    }
}

void setupGoogleSheet()
{
    // Set the callback for Google API access token generation status (for debug only)
    GSheet.setTokenCallback(tokenStatusCallback);

    // Set the seconds to refresh the auth token before expire (60 to 3540, default is 300 seconds)
    GSheet.setPrerefreshSeconds(10 * 60);

    // Log.infoln("setupGoogleSheet -> PrivateKey: %s", GOOGLE_API_PRIVATE_KEY.c_str());

    // Begin the access token generation for Google API authentication
    GSheet.begin(GOOGLE_CLIENT_EMAIL, GOOGLE_PROJECT_ID, GOOGLE_API_PRIVATE_KEY.c_str());

    Log.infoln("Google Sheet client setup ready");
}

void logTemperature()
{
    Log.infoln("Log temperature task started");

    if (!spreadsheetId.isEmpty())
    {
        // tm now = getTimeNow();

        // char timeString[50];
        // strftime(timeString, sizeof(timeString), "%Y-%m-%d %H:%M:%S", &now);

        char range[50]; // Ensure the buffer is large enough
        int newLastRow = lastRow + MaxMeasurementsCount;
        sprintf(range, "%s!A%d:B%d", spreadsheetTab, newLastRow, newLastRow);

        FirebaseJson valueRange;
        valueRange.add("range", range);
        valueRange.add("majorDimension", "ROWS");

        for (int i = 0; i < sizeof(temperatures); i++)
        {
            TmpMeasurement measurement = temperatures[i];

            char col1Path[20];
            sprintf(col1Path, "values/[%d]/[0]", i);

            char col2Path[20];
            sprintf(col2Path, "values/[%d]/[1]", i);

            valueRange.set(col1Path, measurement.time);
            valueRange.set(col2Path, measurement.temperature); // row 1/ column 2
        }

        FirebaseJson response;
        bool success = GSheet.values.update(&response, spreadsheetId, range, &valueRange);
        response.toString(Serial, true);
        Serial.println();

        if (success)
        {
            lastRow = newLastRow;
            measurementIndex = 0;

            // Serial.println("\nGet spreadsheet values...");
            // Serial.println("------------------------------");

            // success = GSheet.values.get(&response /* returned response */, spreadsheetId /* spreadsheet Id to read */, "Sheet1!A1:B10" /* range to read */);
            // response.toString(Serial, true);
            // Serial.println();

            Serial.println(ESP.getFreeHeap());
        }
    }

    logTemperatureTaskComplete = true;

    Log.infoln("Log temperature task completed");
}

tm getTimeNow()
{
    struct tm timeinfo;

    if (!getLocalTime(&timeinfo))
    {
        Serial.println("Failed to obtain time");
    }

    return timeinfo;
}

void createGoogleSheetTab()
{
    if (spreadsheetId.isEmpty())
    {
        Serial.println("Spreadsheet ID is empty. Cannot create tab.");
        return;
    }

    tm now = getTimeNow();
    if (spreadsheetTab[0] != '\0' && now.tm_mday == spreadsheetTabCreated.tm_mday)
    {
        return;
    }

    char timeString[50];
    strftime(timeString, sizeof(timeString), "%Y-%m-%d", &now);

    FirebaseJson response;

    Serial.println("\nCreate tab in spreadsheet...");
    Serial.println("------------------------------");

    FirebaseJson requests;
    FirebaseJsonArray requestsArray;

    FirebaseJson addSheetRequest;
    addSheetRequest.set("addSheet/properties/title", timeString);
    addSheetRequest.set("addSheet/properties/gridProperties/rowCount", 1500);
    addSheetRequest.set("addSheet/properties/gridProperties/columnCount", 2);
    requestsArray.add(addSheetRequest);

    // requests.set("requests", &requestsArray);

    // For Google Sheet API ref doc, go to https://developers.google.com/sheets/api/reference/rest/v4/spreadsheets/batchUpdate
    bool success = GSheet.batchUpdate(&response, spreadsheetId.c_str(), &requestsArray);

    response.toString(Serial, true);
    Serial.println();

    char tabName[50]; // Ensure the buffer is large enough
    sprintf(tabName, "'%s'", timeString);

    if (success)
    {
        Serial.println("Tab created successfully.");
        strcpy(spreadsheetTab, tabName);
        spreadsheetTabCreated = now;
        lastRow = 0;
        measurementIndex = 0;
    }
    else
    {
        Serial.println("Failed to create tab.");

        FirebaseJsonData status;
        response.get(status, "error/status");

        Log.infoln("Create tab error status: %s", status.stringValue.c_str());

        if (status.stringValue == "INVALID_ARGUMENT" && spreadsheetTabCreated.tm_mday == 0)
        {
            strcpy(spreadsheetTab, tabName);
            spreadsheetTabCreated = now;
            Log.infoln("Assuming tab already exists. Setting tab name to: %s", spreadsheetTab);
        }
    }
}

void readLastRow()
{
    if (spreadsheetId.isEmpty())
    {
        Serial.println("Spreadsheet ID is empty. Cannot append row.");
        return;
    }

    char range[50]; // Ensure the buffer is large enough
    sprintf(range, "%s!A:B", spreadsheetTab);

    FirebaseJson response;

    Serial.println("\nAppend row to spreadsheet...");
    Serial.println("------------------------------");

    FirebaseJson valueRange;
    valueRange.add("range", range);
    valueRange.add("majorDimension", "ROWS");
    valueRange.set("values/[0]/[0]", "LastRowCheck");

    // For Google Sheet API ref doc, go to https://developers.google.com/sheets/api/reference/rest/v4/spreadsheets.values/append
    bool success = GSheet.values.append(&response, spreadsheetId, range, &valueRange);

    response.toString(Serial, true);
    Serial.println();

    if (success)
    {
        FirebaseJsonData updatedRange;
        response.get(updatedRange, "updates/updatedRange");

        Log.infoln("Number of rows appended: %s", updatedRange.stringValue.c_str());

        char updatedRangeSubString[50]; // Ensure the buffer is large enough
        sprintf(updatedRangeSubString, "%s!A", spreadsheetTab);

        lastRow = replaceString(updatedRange.stringValue, updatedRangeSubString, "").toInt() - 1;
    }
    else
    {
        Serial.println("Failed to append row.");
    }
}

bool setSecretsFromConfig()
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
    if (readConfig() == false)
    {
        Serial.println("setup -> Could not read Config file");
        return false;
    }

    Log.infoln("Secrets -> UserEmail: %s, ClientEmail: %s", GOOGLE_USER_EMAIL, GOOGLE_CLIENT_EMAIL.c_str());

    return true;
}

bool readConfig()
{
    String fileText = readFile(configFilePath);

    int fileSize = fileText.length();
    Serial.println("Config file size: " + fileSize);

    FirebaseJson json;
    json.setJsonData(fileText);

    GOOGLE_PROJECT_ID = getJsonValue(json, "project_id");

    String privateKeyString = getJsonValue(json, "private_key");
    privateKeyString.replace("\\n", "\n");

    GOOGLE_API_PRIVATE_KEY = privateKeyString;

    GOOGLE_CLIENT_EMAIL = getJsonValue(json, "client_email");

    const char *userEmail = getJsonValue(json, "shared_user_email").c_str();
    GOOGLE_USER_EMAIL = userEmail;

    WIFI_SSID = getJsonValue(json, "wifi_ssid");
    WIFI_PASSWORD = getJsonValue(json, "wifi_password");

    // Log.infoln("Config -> PrivateKey: %s", GOOGLE_API_PRIVATE_KEY);

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

String replaceString(String input, const String &from, const String &to)
{
    String result = input;    // Create a copy of the input string
    result.replace(from, to); // Perform the replacement
    return result;            // Return the modified string
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

bool compareCharByChar(const char *str1, const char *str2, bool asIntPrint)
{
    while (*str1 != '\0' && *str2 != '\0')
    {
        if (*str1 != *str2)
        {
            if (asIntPrint)
            {
                Log.infoln("Chars differ: %d != %d", (int)*str1, (int)*str2);
            }
            else
                Log.infoln("Chars differ: '%c' != '%c'", *str1, *str2);
            return false; // Characters differ
        }
        else
        {
            if (asIntPrint)
            {
                Log.infoln("Chars match: %d == %d", (int)*str1, (int)*str2);
            }
            else
                Log.infoln("Chars match: '%c' == '%c'", *str1, *str2);
        }
        str1++;
        str2++;
    }
    // Check if both strings ended at the same time
    return *str1 == '\0' && *str2 == '\0';
}
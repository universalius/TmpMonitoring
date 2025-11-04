#include <Arduino.h>
#include <ArduinoLog.h>
#include <WiFi.h>
#include <ESP_Google_Sheet_Client.h>
#include <LittleFS.h>
#include <iostream>

#define thermistorPin 1
#define routerPowerOnPin 2

#define configFilePath "/config.json"

#define uS_TO_S_FACTOR 1000000ULL
#define TIME_TO_SLEEP 30 // seconds

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

// void scanMultiplexerAddresses()
// {
//     Wire.begin();
//     if (multiplexer.begin() == false)
//     {
//         Log.info("COULD NOT CONNECT TO MULTIPLEXER \n");

//     for (uint8_t channel = 0; channel < 8; channel++)
//     {
//         multiplexer.enableChannel(channel);
//         Log.info("TCA channel #%d\n", channel);

//         for (uint8_t addr = 0; addr <= 127; addr++)
//         {
//             if (addr == multiplexerAddress)
//                 continue;

//             Wire.beginTransmission(addr);
//             if (!Wire.endTransmission())
//             {
//                 Log.info("Found I2C 0x%x\n", addr);
//             }
//         }

//         multiplexer.disableChannel(channel);
//     }
//     Log.info("Done scan multiplexer addresses\n");
// }

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
        digitalWrite(routerPowerOnPin, HIGH);
        delay(1000 * 30);

        // createGoogleSheet();

        createGoogleSheetTab();

        if (lastRow == 0)
        {
            readLastRow();
        }

        double temperature = readTemperature();
        logTemperature(temperature);

        digitalWrite(routerPowerOnPin, LOW);
        esp_deep_sleep_start();
    }

    delay(500);
}

// void makeMotorShaftOneTurnTask()
// {
//     multiplexer.enableChannel(1);

//     if (millis() - as5600.encoderTimer > 125) // 125 ms will be able to make 8 readings in a sec which is enough for 60 RPM
//     {
//         as5600.readRawAngle();  // ask the value from the sensor
//         as5600.correctAngle();  // tare the value
//         as5600.checkQuadrant(); // check quadrant, check rotations, check absolute angular position

//         as5600.encoderTimer = millis();

//         if (currentTurns != as5600.numberofTurns)
//         {
//             currentTurns = as5600.numberofTurns;
//             motor.stop();
//             delay(1000);

//             motor.move();

//             Log.info("Turns: %d\n", currentTurns);
//             Log.info("Speed: %F\n", as5600.rpmValue);
//         }

//         /*A little brainstorm on determining the required delay
//          * The above 3 functions require about 300-310 us to finish
//          * They mess up the interrupt of the CNC encoder due to the i2C communication
//          * Therefore it is not good if they are called very often
//          * We want to detect at least every rotations of the shaft
//          * I say (arbitrarily), that we need to detect at least 2 angles in each quadrants, so in 1 turn of the shaft, there are 8 readings
//          * 8 readings per turn can be converted into readings per second based on the expected highest speed
//          * Example:
//          * 60 RPM = 60/60 RPS (rounds per seconds) = 1 RPS
//          * 1 round per second -> 8 reading per second -> 1 second/8 readings = 0.125 s = 125 ms is the frequency of readings
//          *
//          * Example 2:
//          *
//          * 100 RPM = 100/60 = 1.667 RPS
//          * 1 round = 0.599 s -> 0.599 s/ 8 readings = 74.98 ~ 75 ms.
//          * Check: 60/100 = 0.6 -> 75/125 = 0.6.
//          */
//     }

//     as5600.calculateRPMTask(); // calculate the RPM
// }

double readTemperature()
{
    int adcValue = analogRead(thermistorPin);                       // read ADC pin
    double voltage = (float)adcValue / 4095.0 * 3.3;                // calculate voltage
    double Rt = 10 * voltage / (3.3 - voltage);                     // calculate resistance value of thermistor
    double tempK = 1 / (1 / (273.15 + 25) + log(Rt / 10) / 3950.0); // calculate temperature (Kelvin)
    double tempC = tempK - 273.15;                                  // calculate temperature (Celsius)
    Serial.printf("Pin value : %d,\tVoltage : %.2fV, \tTemperature : %.2fC\n", adcValue, voltage, tempC);

    return tempC;
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

void logTemperature(double temperature)
{
    Log.infoln("Log temperature task started");

    if (!spreadsheetId.isEmpty())
    {
        tm now = getTimeNow();

        char timeString[50];
        strftime(timeString, sizeof(timeString), "%Y-%m-%d %H:%M:%S", &now);

        char range[50]; // Ensure the buffer is large enough
        sprintf(range, "%s!A%d:B%d", spreadsheetTab, lastRow + 1, lastRow + 1);

        FirebaseJson valueRange;
        valueRange.add("range", range);
        valueRange.add("majorDimension", "ROWS");
        valueRange.set("values/[0]/[0]", timeString);
        valueRange.set("values/[0]/[1]", temperature); // row 1/ column 2

        FirebaseJson response;
        bool success = GSheet.values.update(&response, spreadsheetId, range, &valueRange);
        response.toString(Serial, true);
        Serial.println();

        if (success)
        {
            lastRow++;

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

// void createGoogleSheet()
// {
//     tm now = getTimeNow();

//     if (!spreadsheetId.isEmpty() && now.tm_mday == spreadsheetTabCreated.tm_mday)
//     {
//         return;
//     }

//     FirebaseJson response;

//     Serial.println("\nCreate spreadsheet...");
//     Serial.println("------------------------");

//     Log.infoln("Creating new spreadsheet for user: %s", GOOGLE_USER_EMAIL);

//     // strftime(timeStringBuff, sizeof(timeStringBuff), "%A, %B %d %Y %H:%M:%S", &timeinfo);

//     // FirebaseJson spreadsheet;
//     // spreadsheet.set("properties/title", "BasementTmp_" + now.tm_year + now.tm_mon + now.tm_mday);
//     // spreadsheet.set("sheets/properties/gridProperties/rowCount", 2000);
//     // spreadsheet.set("sheets/properties/gridProperties/columnCount", 2);
//     // spreadsheet.set("sheets/properties/gridProperties/columnCount", 2);

//     // // For Google Sheet API ref doc, go to https://developers.google.com/sheets/api/reference/rest/v4/spreadsheets/create
//     // bool success = GSheet.create(&response, &spreadsheet, GOOGLE_USER_EMAIL);
//     String fileName = "BasementTmp_" + now.tm_year + now.tm_mon + now.tm_mday;
//     bool success = GSheet.createEmpty(&response, fileName, "1W8HnOToJvbgOWrHg_5CZWhRhjTwWsAlD");

//     response.toString(Serial, true);
//     Serial.println();

//     if (success)
//     {
//         // Get the spreadsheet id from already created file.
//         FirebaseJsonData result;
//         response.get(result, FPSTR("spreadsheetId")); // parse or deserialize the JSON response
//         if (result.success)
//         {
//             spreadsheetId = result.to<const char *>();
//         }

//         // Get the spreadsheet URL.
//         result.clear();
//         response.get(result, FPSTR("spreadsheetUrl")); // parse or deserialize the JSON response
//         if (result.success)
//         {
//             String spreadsheetURL = result.to<const char *>();
//             Serial.println("\nThe spreadsheet URL");
//             Serial.println(spreadsheetURL);
//         }

//         spreadsheetTabCreated = now;
//     }
// }

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
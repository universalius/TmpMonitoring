#include "GoogleTmpLogger.h"
#include <Arduino.h>
#include <ArduinoLog.h>
#include <ESP_Google_Sheet_Client.h>

#define thermistorPin 1
#define routerPowerOnPin 2

#define MaxMeasurementsCount 20

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

tm getTimeNow()
{
    struct tm timeinfo;

    if (!getLocalTime(&timeinfo))
    {
        Serial.println("Failed to obtain time");
    }

    return timeinfo;
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
    Serial.printf("Pin value : %d,\tVoltage : %.2fV, \tTemperature : %.2fC,\tTime : %s\n", adcValue, voltage, tempC, timeString);

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
        sprintf(range, "%s!A%d:B%d", spreadsheetTab, lastRow + 1, newLastRow);

        FirebaseJson valueRange;
        valueRange.add("range", range);
        valueRange.add("majorDimension", "ROWS");

        Log.infoln("Logging to range: %s", range);

        for (int i = 1; i <= MaxMeasurementsCount; i++)
        {
            TmpMeasurement measurement = temperatures[i - 1];

            char col1Path[20];
            sprintf(col1Path, "values/[%d]/[0]", i);

            char col2Path[20];
            sprintf(col2Path, "values/[%d]/[1]", i);

            valueRange.set(col1Path, measurement.time);
            valueRange.set(col2Path, measurement.temperature); // row 1/ column 2

            Log.infoln("Measurement %d -> Time: %s, Temp: %.2f", i, measurement.time, measurement.temperature);
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

String replaceString(String input, const String &from, const String &to)
{
    String result = input;    // Create a copy of the input string
    result.replace(from, to); // Perform the replacement
    return result;            // Return the modified string
}

String getConfigValue(FirebaseJson json, String key)
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

bool setGoogleSecretsFromConfig(FirebaseJson json)
{
    GOOGLE_PROJECT_ID = getConfigValue(json, "project_id");

    String privateKeyString = getConfigValue(json, "private_key");
    privateKeyString.replace("\\n", "\n");

    GOOGLE_API_PRIVATE_KEY = privateKeyString;

    GOOGLE_CLIENT_EMAIL = getConfigValue(json, "client_email");

    const char *userEmail = getConfigValue(json, "shared_user_email").c_str();
    GOOGLE_USER_EMAIL = userEmail;

    Log.infoln("Secrets -> UserEmail: %s, ClientEmail: %s", userEmail, GOOGLE_CLIENT_EMAIL.c_str());

    return true;
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

void GoogleTmpLogger::setup(FirebaseJson json)
{
    setGoogleSecretsFromConfig(json);

    // Set the callback for Google API access token generation status (for debug only)
    GSheet.setTokenCallback(tokenStatusCallback);

    // Set the seconds to refresh the auth token before expire (60 to 3540, default is 300 seconds)
    GSheet.setPrerefreshSeconds(10 * 60);

    // Log.infoln("setupGoogleSheet -> PrivateKey: %s", GOOGLE_API_PRIVATE_KEY.c_str());

    // Begin the access token generation for Google API authentication
    GSheet.begin(GOOGLE_CLIENT_EMAIL, GOOGLE_PROJECT_ID, GOOGLE_API_PRIVATE_KEY.c_str());

    Log.infoln("spreadsheetTab : %s, lastRow: %d, day: %d", spreadsheetTab, lastRow, spreadsheetTabCreated.tm_mday);

    Log.infoln("GoogleTmpLogger setup complete");
}

TmpMeasurement GoogleTmpLogger::logTmpTask()
{
    // Call ready() repeatedly in loop for authentication checking and processing
    bool ready = GSheet.ready();

    while (!ready)
    {
        delay(500);
        ready = GSheet.ready();
    }

    if (measurementIndex >= MaxMeasurementsCount - 1)
    {
        Log.infoln("Max measurements reached, powering on router to log data %d", measurementIndex);

        readAndRememberTemperature();

        digitalWrite(routerPowerOnPin, HIGH);
        delay(1000 * 5);

        createGoogleSheetTab();

        if (lastRow == 0)
        {
            readLastRow();
        }

        logTemperature();

        digitalWrite(routerPowerOnPin, LOW);
    }
    else
    {
        readAndRememberTemperature();
    }

    return temperatures[measurementIndex - 1];
}

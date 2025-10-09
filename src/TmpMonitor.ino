#include <Arduino.h>
#include <ArduinoLog.h>
#include <WiFi.h>
#include <ESP_Google_Sheet_Client.h>

#define thermistorPin 1
#define routerPowerOnPin 2

#define WIFI_SSID "Galaxy S20 FEDF80"
#define WIFI_PASSWORD "kdbi8870"

#define GOOGLE_PROJECT_ID "logical-utility-388206"

// Service Account's client email
#define GOOGLE_CLIENT_EMAIL "esp32test20230529@logical-utility-388206.iam.gserviceaccount.com"

// Your email to share access to spreadsheet
#define GOOGLE_USER_EMAIL "majid.merati.2022@gmail.com"

const char GOOGLE_API_PRIVATE_KEY[] PROGMEM = "-----BEGIN PRIVATE KEY-----\nMIIEvQIBADANBgkqhkiG9w0BAQEFAASCBKcwggSjAgEAAoIBAQCbBLtWIEx2OmTI\nzyNk53HtxjebDfGp0AcbAcC/ztN6CTPYKKO7k1jGcKkSUMe7oWGr4lB/QeS0XBFG\nkzjS3LpIuTMlBtf4erf7fcFqCF5VlYYWwuprOmZe3hyYvlc8oBUIcgQvOA6xQcxW\n0tJTAQXfT9g1IF3lRupZ6mCN1ILinxMQOA7zNLa7CLe2wq1hFX4kn8+YztHAiunp\nR7EMhzZAL9rwR2VpWoFZj24TZpQzEYS1S7n3iD8d0Ip3mVmoNcYDe39I5+ZE8xe1\nuqbc4Z5SzUe6RyytAMLL4tMAqDqvDTXtnD9Hw1f3DTymEWEFYjfjO3B6Pt90VLMG\n48r7veZvAgMBAAECggEAO5XzCGT73woNCV8NhNCt6y6g2xneBV7wDEJ9O30drq/w\n3KqqXQSDQCu34VFppMagi3g/ZtsGNQmanl0in3K2rBN8BESAKYPKSoIJIV+8GvuR\nghhEERlulhKcPV4UnDoQO6HP1/KdOydGiy0YKU2gWzkOq0UYsnOyywMbQAkqkj7n\n5ELfR//Leewx92zTiGrI4xns0sDm15rEYQvF8WQZeMFUblmJ+5FfuSK5hP0SaX4F\nvv9cx34TteUm1sro+v65HYTjY8Ozcur+ZjiuSNuAx4vs7CmpUM3kAjIN2451H9ka\ny3PY7I3bvFhy9tIHmQflFiokAFk3eH//HavpmBnroQKBgQDKJYElbW1b9L+5RHK4\nD8vRRbIgNVw1lcsBulwc4/VyAUORd3V9JQwfGvGBKykTVvJ75USR9T49BIDBZeR+\nfeO2RZtCaEXm5OtQgVM8p9GFUxhRk283CPf/uZbMkNkXXCLefcVWCBr+6+2extUV\nz+MMgghwFx4vjbJqe2RqFtA+vwKBgQDEURJ6zhKu7pJy1wNlPkfsnDwb2sbj/UmG\nmzA+9KEP9PQExxShRYBwyrsgGRB644kLtWv6tOTezib54SnWXAcVHXkkP1hWrw79\nfWY9tgPldwvlT9KMXTiG6NBHDZD/lwVtiMqulmKpXVr+AVSOp+t/vzquvOO8MKl2\nP5g5BvT0UQKBgQCX1S24uAWBIUd0V7Y3DB5R9KCDUrEMypMtnBWd2Zc85zgiJfEI\na3A8WNd25r02eoMtjho/602xNRWkrey/Gb2U5Zller6uW9lnoYusrnxQQQauFyhc\n1Q65dGORCWcWBa1nIl67bOEiAoF7VestM/VdKro9vw98NlkBY+cbTEFkLwKBgHno\nglpCR7XbrpOj05b2UwGqj2UYuXNwoUz2Z8JgdPgMu8+PtM7k8vqX2hX3mpVFYKAl\nu/UGii2VxPxdB3b21OsWz1hkAb5qnRtq2KevtMG06G0w4WWKqgSU5GmLEiS1qog3\nbu11s7TKpSugtdqUihhdLtq9r4n4ifNFUIjxOgHRAoGAbMlZi6KtyuIqfpnOvYej\nIO2SmKA85EBelrZCl2xKlIgGpLRITzfiQ5k+HH3ts22iw3ld+tBxpg82EX4Zuavj\niM57EIf6hzLsh8eLpic85ziD6PV14PBQUECS1Bb14fMJ4k3rcGcNFo2o9tgjlu9R\nKDx2yXeNgh/pToM7GLvlIOg=\n-----END PRIVATE KEY-----\n";

bool logTemperatureTaskComplete = false;

String spreadsheetId;

tm spreadsheetCreated;

int lastRow = 1;

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

    connectToWiFi();

    setupGoogleSheet();

    char *ntpServer = "pool.ntp.org";
    long gmtOffset_sec = 19800;
    int daylightOffset_sec = 0;
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
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

    // Begin the access token generation for Google API authentication
    GSheet.begin(GOOGLE_CLIENT_EMAIL, GOOGLE_PROJECT_ID, GOOGLE_API_PRIVATE_KEY);
}

void logTemperature(double temperature)
{
    if (!spreadsheetId.isEmpty())
    {
        tm now = getTimeNow();

        char timeString[50];
        strftime(timeString, sizeof(timeString), "%Y-%m-%d %H:%M:%S", &now);

        char range[50]; // Ensure the buffer is large enough
        sprintf(range, "Sheet1!A%d:B%d", lastRow + 1, lastRow + 1);

        FirebaseJson valueRange;
        valueRange.add("range", range);
        valueRange.add("majorDimension", "ROWS");
        valueRange.set("values/[0]/[0]", timeString);
        valueRange.set("values/[0]/[1]", temperature); // row 1/ column 2

        FirebaseJson response;
        bool success = GSheet.values.update(&response, spreadsheetId, range, &valueRange);
        response.toString(Serial, true);
        Serial.println();

        // for (int counter = 0; counter < 10; counter++)
        // {
        //     Serial.println("\nUpdate spreadsheet values...");
        //     Serial.println("------------------------------");
        //     // if (!getLocalTime(&timeinfo))
        //     // {
        //     //     Serial.println("Failed to obtain time");
        //     //     return;
        //     // }
        //     // strftime(timeStringBuff, sizeof(timeStringBuff), "%A, %B %d %Y %H:%M:%S", &timeinfo);
        //     // asString = timeStringBuff;
        //     // asString.replace(" ", "-");
        //     // SensValue = analogRead(34);
        //     // itoa(SensValue, numberArray, 10);

        //     sprintf(buffer, "values/[%d]/[1]", counter);
        //     valueRange.set(buffer, numberArray);

        //     sprintf(buffer, "values/[%d]/[0]", counter);
        //     valueRange.set(buffer, asString);

        //     // sprintf(buffer, "Sheet1!A%d:B%d", 1 + counter, 10 + counter);

        //     success = GSheet.values.update(&response /* returned response */, spreadsheetId /* spreadsheet Id to update */, "Sheet1!A1:B1000" /* range to update */, &valueRange /* data to update */);
        //     response.toString(Serial, true);
        //     Serial.println();
        //     // valueRange.clear();
        //     delay(5000);
        // }

        // For Google Sheet API ref doc, go to https://developers.google.com/sheets/api/reference/rest/v4/spreadsheets.values/update

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

void createGoogleSheet()
{
    tm now = getTimeNow();

    if (!spreadsheetId.isEmpty() && now.tm_mday == spreadsheetCreated.tm_mday)
    {
        return;
    }

    FirebaseJson response;

    Serial.println("\nCreate spreadsheet...");
    Serial.println("------------------------");

    // strftime(timeStringBuff, sizeof(timeStringBuff), "%A, %B %d %Y %H:%M:%S", &timeinfo);

    FirebaseJson spreadsheet;
    spreadsheet.set("properties/title", "BasementTmp_" + now.tm_year + now.tm_mon + now.tm_mday);
    spreadsheet.set("sheets/properties/gridProperties/rowCount", 2000);
    spreadsheet.set("sheets/properties/gridProperties/columnCount", 2);

    // For Google Sheet API ref doc, go to https://developers.google.com/sheets/api/reference/rest/v4/spreadsheets/create
    bool success = GSheet.create(&response, &spreadsheet, GOOGLE_USER_EMAIL);
    response.toString(Serial, true);
    Serial.println();

    if (success)
    {
        // Get the spreadsheet id from already created file.
        FirebaseJsonData result;
        response.get(result, FPSTR("spreadsheetId")); // parse or deserialize the JSON response
        if (result.success)
        {
            spreadsheetId = result.to<const char *>();
        }

        // Get the spreadsheet URL.
        result.clear();
        response.get(result, FPSTR("spreadsheetUrl")); // parse or deserialize the JSON response
        if (result.success)
        {
            String spreadsheetURL = result.to<const char *>();
            Serial.println("\nThe spreadsheet URL");
            Serial.println(spreadsheetURL);
        }

        spreadsheetCreated = now;
    }
}

void loop()
{
    digitalWrite(routerPowerOnPin, HIGH);
    delay(1000 * 60);

    // Call ready() repeatedly in loop for authentication checking and processing
    bool ready = GSheet.ready();
    if (ready)
    {
        createGoogleSheet();

        double temperature = readTemperature();
        logTemperature(temperature);
    }

    digitalWrite(routerPowerOnPin, LOW);
    delay(1000 * 60 * 2);
}
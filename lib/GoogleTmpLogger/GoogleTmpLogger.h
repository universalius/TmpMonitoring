#ifndef GoogleTmpLogger_H
#define GoogleTmpLogger_H

#include <Arduino.h>
#include <ArduinoLog.h>
#include <ESP_Google_Sheet_Client.h>

struct TmpMeasurement
{
    double temperature;
    char time[20];
};

class GoogleTmpLogger
{
public:
    TmpMeasurement logTmpTask();
    void setup(FirebaseJson json);
};

#endif // GoogleTmpLogger_H
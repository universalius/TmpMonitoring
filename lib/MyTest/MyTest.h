#ifndef MYTEST_H
#define MYTEST_H

#include <L298N.h>
#include <Arduino.h>
#include <ArduinoLog.h>

class MyTest
{
private:
public:
    // Constructor
    MyTest();

    void makeOneStepTask(Direction direction);
    int _mytestIndex2; // MyTest index for debugging
};

#endif // MYTEST_H
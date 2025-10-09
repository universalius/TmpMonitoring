#include "MyTest.h"

// Constructor
// MyTest::MyTest() : multiplexer(TCA9548()) {}

MyTest::MyTest()
{
    Log.info("MyTest constructor WTF.\n");
}

// Method to make one step
void MyTest::makeOneStepTask(Direction direction)
{
    // Log.info("Current mytest index: %d for name %s\n", _mytestIndex2, encoder.name);

    Serial.print("Current mytest index from serial: ");
    Serial.println(_mytestIndex2, 0); // number of turns in absolute terms (can be negative which indicates CCW turns)

    int a = _mytestIndex2 + 1; // Increment the index for the next step

    Serial.print("Current a: ");
    Serial.println(a, 0); // number of turns in absolute terms (can be negative which indicates CCW turns)

    _mytestIndex2 = a;
    //_mytestIndex2++;

    if (_mytestIndex2 > 20)
    {
        Log.info("MyTest index exceeded 20, returning.\n");
    }
    else
    {
        Log.info("MyTest index is within range, proceeding with one step task.\n");
    }

    delay(1000);
}

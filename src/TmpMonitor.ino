#include <Arduino.h>
#include "L298N.h"
#include "AS5600.h"
#include <DabbleESP32.h>
#include <ArduinoLog.h>
#include "TCA9548.h"
#include "Hexapod.h"
#include "MyTest.h"
#define CUSTOM_SETTINGS
#define INCLUDE_GAMEPAD_MODULE

int multiplexerAddress = 0x70;

TCA9548 multiplexer(multiplexerAddress);

Hexapod hexapod(multiplexer);

MyTest test; // Create a Test object

int currentTurns = 0;

void scanMultiplexerAddresses()
{
    Wire.begin();
    if (multiplexer.begin() == false)
    {
        Log.info("COULD NOT CONNECT TO MULTIPLEXER \n");
    }

    for (uint8_t channel = 0; channel < 8; channel++)
    {
        multiplexer.enableChannel(channel);
        Log.info("TCA channel #%d\n", channel);

        for (uint8_t addr = 0; addr <= 127; addr++)
        {
            if (addr == multiplexerAddress)
                continue;

            Wire.beginTransmission(addr);
            if (!Wire.endTransmission())
            {
                Log.info("Found I2C 0x%x\n", addr);
            }
        }

        multiplexer.disableChannel(channel);
    }
    Log.info("Done scan multiplexer addresses\n");
}

void setup()
{
    Serial.begin(115200); // make sure your Serial Monitor is also set at this baud rate.
    Log.begin(LOG_LEVEL_VERBOSE, &Serial);
    //Dabble.begin("HexapodEsp32"); // set bluetooth name of your device
    scanMultiplexerAddresses();

    hexapod.legs[0].setup(); // setup the first leg
    // hexapod.setup();                     // setup the hexapod
    // hexapod.moveLegsToInitialPosition(); // move the legs to the initial position
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

void loop()
{
    // Serial.print("y_axis: ");
    // delay(1000);

    auto &leg = hexapod.legs[0];   // Get the first leg for testing
    leg.makeOneStepTask(FORWARD); // make one step with the first leg

    if (leg.stepCompleted)
    {
        delay(2000);
        leg.stepCompleted = false; // Reset stepCompleted for the next step
        Log.info("Step completed for leg %s\n", leg.encoder.name);
    }

    // // motor.forward();
    // // delay(2000);
    // // motor.stop();
    // // delay(1000);
    // // motor.backward();
    // // delay(2000);
    // // motor.stop();
    // // delay(1000);

    // auto direction = FORWARD;
    // bool moveEnabled = false;
    // String command = "";
    // String prevCommand = "";

    // Dabble.processInput(); // this function is used to refresh data obtained from smartphone.Hence calling this function is mandatory in order to get data properly from your mobile.
    // if (GamePad.isUpPressed())
    // {
    //     // Log.info("Up\n");
    //     // motor.stop();
    //     // delay(500);
    //     // motor.forward();
    //     direction = FORWARD;
    //     command = "Up";
    // }

    // if (GamePad.isDownPressed())
    // {
    //     // Log.info("Down\n");
    //     // motor.stop();
    //     // delay(500);
    //     // motor.backward();
    //     direction = BACKWARD;
    //     command = "Down";
    // }

    // // // if (GamePad.isLeftPressed())
    // // // {
    // // //     Serial.print("Left");
    // // // }

    // // // if (GamePad.isRightPressed())
    // // // {
    // // //     Serial.print("Right");
    // // // }

    // // if (GamePad.isSquarePressed())
    // // {
    // //     int speed = mapSpeed(100);
    // //     Log.info("Square: %d\n", speed);
    // //     motor.setSpeed(speed);
    // // }

    // // if (GamePad.isTrianglePressed())
    // // {
    // //     int speed = mapSpeed(75);
    // //     Log.info("Triangle: %d\n", speed);
    // //     motor.setSpeed(speed);
    // // }

    // // if (GamePad.isCirclePressed())
    // // {
    // //     int speed = mapSpeed(50);
    // //     Log.info("Circle: %d\n", speed);
    // //     motor.setSpeed(speed);
    // // }

    // // if (GamePad.isCrossPressed())
    // // {
    // //     int speed = mapSpeed(20);
    // //     Log.info("Cross: %d\n", speed);
    // //     motor.setSpeed(speed);
    // // }

    // // if (GamePad.isStartPressed())
    // // {
    // //     Log.info("Start\n");
    // //     motor.stop();
    // // }

    // hexapod.moveWithTripodGateTask(direction,
    //                                (command == "Up" || command == "Down") && command == prevCommand);

    // prevCommand = command;

    // // makeMotorShaftOneTurnTask();

    // // if (GamePad.isSelectPressed())
    // // {
    // //     Serial.print("Select");
    // // }
    // // Serial.print('\t');

    // // int a = GamePad.getAngle();
    // // Serial.print("Angle: ");
    // // Serial.print(a);
    // // Serial.print('\t');
    // // int b = GamePad.getRadius();
    // // Serial.print("Radius: ");
    // // Serial.print(b);
    // // Serial.print('\t');
    // // float c = GamePad.getXaxisData();
    // // Serial.print("x_axis: ");
    // // Serial.print(c);
    // // Serial.print('\t');
    // // float d = GamePad.getYaxisData();
    // // Serial.print("y_axis: ");
    // // Serial.println(d);
}
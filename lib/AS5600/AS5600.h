#ifndef AS5600_H
#define AS5600_H

#include <Arduino.h>
#include <Wire.h> //This is for i2C
#include <ArduinoLog.h>

struct EncoderQuadrant {
    int current;
    int previous;
};

class AS5600
{
public:
    AS5600(String name, float zeroAngle = 0.0); // constructor
    void setup();
    String name;
    float readDegreeAngle();
    float getTaredDegreeAngle();
    int getQuadrant(float taredDegreeAngle);
    EncoderQuadrant setQuadrant(float taredDegreeAngle);
    bool didOneTurn(EncoderQuadrant quadrant);
    void checkMagnetPresence();
    void calculateRPMTask();
    float encoderTimer = 0;
    float numberofTurns = 0; // number of turns
    float rpmValue = 0;      // RPM value
    float zeroAngle = 0;     // zero angle (for taring)

private:
    // Magnetic sensor things
    int _magnetStatus = 0; // value of the status register (MD, ML, MH)

    int _lowbyte;    // raw angle 7:0
    word _highbyte;  // raw angle 7:0 and 11:8
    int _rawAngle;   // final raw angle
    float _degAngle; // raw angle in degrees (360/4096 * [value between 0-4095])

    int _quadrantNumber;
    int _previousquadrantNumber = 111; 
    float _correctedAngle = 0;                    // tared angle - based on the startup value
    float _startAngle = 0;                        // starting angle
    float _totalAngle = 0;                        // total absolute angular displacement
    float _previoustotalAngle = 0;                // for the display printing

    // RPM calculation
    float _rpmInterval = 200;    // RPM is calculated every 0.2 seconds
    float _rpmTimer = 0;         // Timer for the rpm
    float _rpmTimerdiff = 0;     // Time difference for more exact rpm calculation
    float _recentTotalAngle = 0; // Total angle from the previous calculation

    int _statusRegisterAddress;   // status register
    int _wireAddress;             // i2C address
    int _rawAngleLowByteAddress;  // raw angle 7:0 address
    int _rawAngleHighByteAddress; // raw angle 11:8 address

int _test = 0; // test variable

    int readByte(int address);
    void calculateRPM();
};

#endif // AS5600_H
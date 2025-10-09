#include "AS5600.h"

AS5600::AS5600(String name, float zeroAngle) // constructor
{
    _wireAddress = 0x36;             // i2C address
    _statusRegisterAddress = 0x0B;   // status register address
    _rawAngleLowByteAddress = 0x0D;  // raw angle 7:0 address
    _rawAngleHighByteAddress = 0x0C; // raw angle 11:8 address
    this->zeroAngle = zeroAngle;     // set the zero angle
    this->name = name;               // set the name

    Log.info("AS5600 constructor WTF\n");
}

void AS5600::setup()
{
    Wire.begin();           // start i2C
    Wire.setClock(800000L); // fast clock
    // General remark on i2C: it seems that the i2C interferes with the attachInterrupt() in some way causing
    // strange readings if the i2C-related hardware is read too often (in every loop iteration).

    checkMagnetPresence();           // check the magnet (blocks until magnet is found)
    _startAngle = readDegreeAngle(); // update startAngle with degAngle - for taring
}

float AS5600::readDegreeAngle()
{
    // 7:0 - bits
    auto lowbyte = readByte(_rawAngleLowByteAddress);

    // 11:8 - 4 bits
    auto highByteInt = readByte(_rawAngleHighByteAddress);

    // 4 bits have to be shifted to its proper place as we want to build a 12-bit number
    word highbyte = highByteInt << 8; // shifting to left
    // What is happening here is the following: The variable is being shifted by 8 bits to the left:
    // Initial value: 00000000|00001111 (word = 16 bits or 2 bytes)
    // Left shifting by eight bits: 00001111|00000000 so, the high byte is filled in

    // Finally, we combine (bitwise OR) the two numbers:
    // High: 00001111|00000000
    // Low:  00000000|00001111
    //       -----------------
    // H|L:  00001111|00001111
    auto rawAngle = highbyte | lowbyte; // int is 16 bits (as well as the word)

    // We need to calculate the angle:
    // 12 bit -> 4096 different levels: 360° is divided into 4096 equal parts:
    // 360/4096 = 0.087890625
    // Multiply the output of the encoder with 0.087890625
    float degAngle = rawAngle * 0.087890625;

    // Serial.print("Deg angle: ");
    // Serial.println(degAngle, 2); //absolute position of the encoder within the 0-360 circle

    return degAngle; // return the angle
}

float AS5600::getTaredDegreeAngle()
{
    auto degAngle = readDegreeAngle();          // read the raw angle from the sensor
    auto correctedAngle = degAngle - zeroAngle; // this tares the position

    if (correctedAngle < 0) // if the calculated angle is negative, we need to "normalize" it
    {
        correctedAngle = _correctedAngle + 360; // correction for negative numbers (i.e. -15 becomes +345)
    }
    else
    {
        // do nothing
    }
    // Serial.print("Corrected angle: ");
    // Serial.println(correctedAngle, 2); //print the corrected/tared angle

    return correctedAngle;
}

int AS5600::getQuadrant(float taredDegreeAngle)
{
    /*
    //Quadrants:
    4  |  1
    ---|---
    3  |  2
    */

    int quadrantNumber = 0; // quadrant number

    // Quadrant 1
    if (taredDegreeAngle >= 0 && taredDegreeAngle <= 90)
    {
        quadrantNumber = 1;
    }

    // Quadrant 2
    if (taredDegreeAngle > 90 && taredDegreeAngle <= 180)
    {
        quadrantNumber = 2;
    }

    // Quadrant 3
    if (taredDegreeAngle > 180 && taredDegreeAngle <= 270)
    {
        quadrantNumber = 3;
    }

    // Quadrant 4
    if (taredDegreeAngle > 270 && taredDegreeAngle < 360)
    {
        quadrantNumber = 4;
    }
    // Serial.print("Quadrant: ");
    // Serial.println(quadrantNumber); //print our position "quadrant-wise"

    return quadrantNumber;

    // if (_quadrantNumber != _previousquadrantNumber) // if we changed quadrant
    // {
    //     if (_quadrantNumber == 1 && _previousquadrantNumber == 4)
    //     {
    //         numberofTurns++; // 4 --> 1 transition: CW rotation
    //     }

    //     if (_quadrantNumber == 4 && _previousquadrantNumber == 1)
    //     {
    //         numberofTurns--; // 1 --> 4 transition: CCW rotation
    //     }
    //     // this could be done between every quadrants so one can count every 1/4th of transition

    //     _previousquadrantNumber = _quadrantNumber; // update to the current quadrant
    // }
    // // Serial.print("Turns: ");
    // // Serial.println(numberofTurns,0); //number of turns in absolute terms (can be negative which indicates CCW turns)

    // // after we have the corrected angle and the turns, we can calculate the total absolute position
    // _totalAngle = (numberofTurns * 360) + _correctedAngle; // number of turns (+/-) plus the actual angle within the 0-360 range
    // // Serial.print("Total angle: ");
    // // Serial.println(totalAngle, 2); //absolute position of the motor expressed in degree angles, 2 digits
}

EncoderQuadrant AS5600::setQuadrant(float taredDegreeAngle)
{
    auto quadrantNumber = getQuadrant(taredDegreeAngle);
    auto prevQuadrantNumberTmp = _previousquadrantNumber; // store the previous quadrant number temporarily

    //Log.info("Quadrant: %d, Previous Quadrant: %d\n", quadrantNumber, _previousquadrantNumber);

    if (quadrantNumber != _previousquadrantNumber) // if we changed quadrant
    {
        _previousquadrantNumber = quadrantNumber; // update to the current quadrant
    }

    return {quadrantNumber, prevQuadrantNumberTmp}; // return the current and previous quadrant numbers
}

bool AS5600::didOneTurn(EncoderQuadrant quadrant)
{
    //Log.info("Current Quadrant: %d, Previous Quadrant: %d\n", quadrant.current, quadrant.previous);
    return (quadrant.current == 1 && quadrant.previous == 4) ||
           (quadrant.current == 4 && quadrant.previous == 1);
}

bool isBitSet(byte value, int bitPosition)
{
    return (value & (1 << bitPosition)) != 0;
}

String intToByteString(int value)
{
    String byteString = "";
    for (int i = 8; i >= 0; i--)
    {
        byteString += isBitSet(value, i) ? "1" : "0";
    }

    return byteString;
}

void AS5600::checkMagnetPresence()
{
    // This function runs in the setup() and it locks the MCU until the magnet is not positioned properly
    int magnetStatus = 0;             // value of the status register (MD, ML, MH)
    while ((magnetStatus & 32) != 32) // while the magnet is not adjusted to the proper distance - 32: MD = 1
    {
        magnetStatus = readByte(_statusRegisterAddress);                                 // read the status register
        Log.info("Magnet status: %d %s\n", magnetStatus, intToByteString(magnetStatus)); // print it in binary so you can compare it to the table (fig 21)
    }

    // Status register output: 0 0 MD ML MH 0 0 0
    // MH: Too strong magnet - 100111 - DEC: 39
    // ML: Too weak magnet - 10111 - DEC: 23
    // MD: OK magnet - 110111 - DEC: 55

    // Serial.println("Magnet found!");
    delay(1000);
}

int AS5600::readByte(int address)
{
    Wire.beginTransmission(_wireAddress); // connect to the sensor

    Wire.write(address);               // figure 21 - register map: Raw angle (7:0)
    Wire.endTransmission();            // end transmission
    Wire.requestFrom(_wireAddress, 1); // request from the sensor

    while (Wire.available() == 0)
        ;               // wait until it becomes available
    return Wire.read(); // Reading the data after the request
}

void AS5600::calculateRPMTask()
{
    // This function calculates the RPM based on the elapsed time and the angle of rotation.
    // Positive RPM is CW, negative RPM is CCW. Example: RPM = 300 - CW 300 rpm, RPM = -300 - CCW 300 rpm.
    _rpmTimerdiff = millis() - _rpmTimer;

    if (_rpmTimerdiff > _rpmInterval)
    {
        calculateRPM();       // Calculate the RPM
        _rpmTimer = millis(); // Update the timer with the current millis() value
    }
}

void AS5600::calculateRPM()
{
    // rpmValue = (60000.0/rpmInterval) * (totalAngle - recentTotalAngle)/360.0;
    rpmValue = (60000.0 / _rpmInterval) * (_totalAngle - _recentTotalAngle) / 360.0;
    // Formula: (60000/2000) * (3600 - 0) / 360;
    // 30 * 10 = 300 RPM. So, in 2 seconds we did 10 turns (3600degrees total angle), then assuming that the speed is constant
    // 2 second is 1/30th of a minute, so we multiply the 2 second data with 30 -> 30*10 = 300 rpm.
    // The purpose of the (60000/rpmInterval) is that we always normalize the rounds per rpmInterval to rounds per minute

    _recentTotalAngle = _totalAngle; // Make the totalAngle as the recent total angle.
}
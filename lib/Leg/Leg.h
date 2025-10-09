#ifndef LEG_H
#define LEG_H

#include "L298N.h"
#include "AS5600.h"
#include <TCA9548.h>

class Leg
{
private:
    TCA9548 multiplexer;
    int oneStepTaskIndex = 0; // Index for the one step task
    int _testIndex1 = 0; // Test index for debugging
public:
    L298N motor;
    AS5600 encoder;
    uint8_t channel;
    bool stepCompleted;

    // Constructor
    // Leg();
    Leg(L298N motor, AS5600 encoder, uint8_t channel, TCA9548 multiplexer);

    void setup();
    void moveLegToInitialPosition();
    void moveLegToPosition(float targetAngel, Direction direction);
    void makeOneStepTask(Direction direction);
    void stopLeg();
    float getEncoderAngle();
};

#endif // LEG_H
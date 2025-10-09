#ifndef HEXAPOD_H
#define HEXAPOD_H

#include "TCA9548.h"
#include <ArduinoLog.h>
#include <Leg.h>

enum LegRotation
{
    CLOCKWHISE,
    COUNTERCLOCKWHISE,
};

class Hexapod
{
private:
    Leg legFrontLeft;
    Leg legFrontRight;
    Leg legMiddleLeft;
    Leg legMiddleRight;
    Leg legBackLeft;
    Leg legBackRight;
    TCA9548 multiplexer;

    Leg tripod1[3];
    Leg tripod2[3];

    void moveTripodLegs(Leg tripod[3], LegRotation rotation);
    bool allLegsStepCompleted();

public:
    // Constructor
    Hexapod(TCA9548 multiplexer);

    // Array of legs
    Leg legs[6];

    // Methods
    void setup();
    void moveLegsToInitialPosition();
    void moveWithTripodGateTask(LegRotation rotation, bool enabled);
};

#endif // HEXAPOD_H
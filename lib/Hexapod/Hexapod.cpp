#include "Hexapod.h"
#include <Wire.h>

// Constructor
Hexapod::Hexapod(TCA9548 multiplexer) : multiplexer(multiplexer),
                                        legFrontLeft(L298N(5, 17, 16), AS5600("front-left"), 7, multiplexer),
                                        legFrontRight(L298N(4, 0, 2), AS5600("front-right"), 2, multiplexer),
                                        legMiddleLeft(L298N(25, 33, 32), AS5600("middle-left"), 6, multiplexer),
                                        legMiddleRight(L298N(19, 18, 23), AS5600("middle-right"), 2, multiplexer),
                                        legBackLeft(L298N(15, 13, 12), AS5600("back-left"), 5, multiplexer),
                                        legBackRight(L298N(14, 27, 26), AS5600("back-right"), 3, multiplexer),
                                        legs{
                                            legFrontLeft,
                                            legFrontRight,
                                            legMiddleLeft,
                                            legMiddleRight,
                                            legBackLeft,
                                            legBackRight},
                                        tripod1{legFrontLeft, legMiddleRight, legBackLeft},
                                        tripod2{legFrontRight, legMiddleLeft, legBackRight}

{
    // Ensure all legs are initialized properly
    // legFrontLeft = Leg(L298N(5, 17, 16), AS5600("front-left"), 7, multiplexer);
    // legFrontRight = Leg(L298N(4, 0, 2), AS5600("front-right"), 2, multiplexer);
    // legMiddleLeft = Leg(L298N(25, 33, 32), AS5600("middle-left"), 6, multiplexer);
    // legMiddleRight = Leg(L298N(3, 1, 23), AS5600("middle-right"), 2, multiplexer);
    // legBackLeft = Leg(L298N(15, 13, 12), AS5600("back-left"), 5, multiplexer);
    // legBackRight = Leg(L298N(14, 27, 26), AS5600("back-right"), 3, multiplexer);

    // Fill the legs array
    // legs = {
    //     legFrontLeft,
    //     legFrontRight,
    //     legMiddleLeft,
    //     legMiddleRight,
    //     legBackLeft,
    //     legBackRight
    // };
    // legs[0] = legFrontLeft;
    // legs[1] = legFrontRight;
    // legs[2] = legMiddleLeft;
    // legs[3] = legMiddleRight;
    // legs[4] = legBackLeft;
    // legs[5] = legBackRight;

    // tripod1[0] = legFrontLeft;
    // tripod1[1] = legMiddleRight;
    // tripod1[2] = legBackLeft;

    // tripod2[0] = legFrontRight;
    // tripod2[1] = legMiddleLeft;
    // tripod2[2] = legBackRight;
}

// Method to set up the Hexapod
void Hexapod::setup()
{
    for (auto &leg : legs)
    {
        leg.setup();
    }

    Log.info("Hexapod setup complete.\n");
}

// Method to set up the Hexapod
void Hexapod::moveLegsToInitialPosition()
{
    for (auto &leg : legs)
    {
        leg.moveLegToInitialPosition();
    }

    tripod1[0].moveLegToPosition(180, FORWARD);
    tripod1[1].moveLegToPosition(180, BACKWARD);
    tripod1[2].moveLegToPosition(180, BACKWARD);

    Log.info("Hexapod setup complete.\n");
}

void Hexapod::moveWithTripodGateTask(LegRotation rotation, bool enabled)
{
    if (!enabled)
    {
        for (auto &leg : legs)
        {
            leg.stopLeg();
        }

        return;
    }

    moveTripodLegs(tripod1, rotation);
    moveTripodLegs(tripod2, rotation == CLOCKWHISE ? COUNTERCLOCKWHISE : CLOCKWHISE);

    if (allLegsStepCompleted())
    {
        delay(2000); // Small delay to avoid busy waiting
        for (auto &leg : legs)
        {
            leg.stepCompleted = false; // Reset stepCompleted for all legs
        }
    }
}

void Hexapod::moveTripodLegs(Leg tripod[3], LegRotation rotation)
{
    tripod[0].makeOneStepTask(rotation == CLOCKWHISE ? BACKWARD : FORWARD);
    tripod[1].makeOneStepTask(rotation == CLOCKWHISE ? FORWARD : BACKWARD);
    tripod[2].makeOneStepTask(rotation == CLOCKWHISE ? BACKWARD : FORWARD);
}

bool Hexapod::allLegsStepCompleted()
{
    for (auto &leg : legs)
    {
        if (!leg.stepCompleted)
        {
            return false; // If any leg's stepCompleted is false, return false
        }
    }
    return true; // All legs have stepCompleted as true
}

// // Method to move all legs
// void Hexapod::moveAllLegs(int angle)
// {
//     legFrontLeft.move(angle);
//     legFrontRight.move(angle);
//     legMiddleLeft.move(angle);
//     legMiddleRight.move(angle);
//     legBackLeft.move(angle);
//     legBackRight.move(angle);
// }
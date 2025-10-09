#include "Leg.h"
#include <Wire.h>

// Constructor
// Leg::Leg() : multiplexer(TCA9548()) {}

Leg::Leg(L298N motor, AS5600 encoder, uint8_t channel, TCA9548 multiplexer)
    : motor(motor), encoder(encoder), channel(channel), multiplexer(multiplexer)
{

    Log.info("Leg constructor WTF.\n");
}

// Method to set up the Leg
void Leg::setup()
{
    motor.setSpeed(100); // Set maximum speed
    multiplexer.enableChannel(channel);
    encoder.setup();
    multiplexer.disableChannel(channel);

    Log.info("Leg setup complete.\n");
}

// Method to set up the Leg
void Leg::moveLegToInitialPosition()
{
    auto angle = getEncoderAngle();
    if (angle >= 359 || (angle >= 0 && angle <= 1))
    {
        return; // No need to move if already at 0 degrees
    }

    auto quadrant = encoder.getQuadrant(angle);
    auto direction = (quadrant == 1 || quadrant == 2) ? BACKWARD : FORWARD;

    moveLegToPosition(0, direction);

    Log.info("Leg movement to Initial position complete %s.\n", encoder.name);
}

void Leg::moveLegToPosition(float targetAngel, Direction direction)
{
    motor.direction = direction;
    motor.move();
    auto angle = getEncoderAngle();

    while (angle != targetAngel)
    {
        angle = getEncoderAngle();
        if (angle >= targetAngel - 1 && angle <= targetAngel + 1)
        {
            motor.stop();
            break;
        }
        delay(100); // Small delay to avoid busy waiting
    }

    Log.info("Leg movement to target position complete.\n");
}

// Method to make one step
void Leg::makeOneStepTask(Direction direction)
{
    // Log.info("Current test index: %d for name %s\n", _testIndex1, encoder.name);

    // _testIndex1++;

    // if (_testIndex1 > 20)
    // {
    //     Log.info("Test index exceeded 20, returning.\n");
    // }
    // else{
    //     Log.info("Test index is within range, proceeding with one step task.\n");
    // }


    // delay(1000);

    if (stepCompleted)
    {
        return;
    }

    if (oneStepTaskIndex == 0)
    {
        encoder.encoderTimer = 0; // Reset the timer
        motor.direction = direction;
        motor.move();
    }

    if (millis() - encoder.encoderTimer > 125) // 125 ms will be able to make 8 readings in a sec which is enough for 60 RPM
    {
        encoder.encoderTimer = millis();

        auto angle = getEncoderAngle();
        auto quadrant = encoder.setQuadrant(angle); // Set the current quadrant

        // Log.info("Current index: %d\n", oneStepTaskIndex);

        oneStepTaskIndex++; // Increment the index for the next step

        if (encoder.didOneTurn(quadrant))
        {
            motor.stop();
            stepCompleted = true; // Step completed
            oneStepTaskIndex = 0; // Reset the index for the next step

            Log.info("One step completed for leg %s on angle %F.\n", encoder.name, angle);
            return; // Step completed
        }
    }
}

void Leg::stopLeg()
{
    motor.stop();
    stepCompleted = false;
    oneStepTaskIndex = 0;
}

float Leg::getEncoderAngle()
{
    multiplexer.enableChannel(channel);
    auto angle = encoder.getTaredDegreeAngle();
    multiplexer.disableChannel(channel);

    Log.info("Angle: %F\n", angle);

    return angle;
}

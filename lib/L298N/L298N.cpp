#include "L298N.h"

L298N::L298N(int enPin, int in1Pin, int in2Pin)
{
    _enPin = enPin;
    _in1Pin = in1Pin;
    _in2Pin = in2Pin;
    _speed = 0;
    direction = FORWARD;

    // if (enPin == 19)
    // {
        pinMode(_enPin, OUTPUT);
        pinMode(_in1Pin, OUTPUT);
        pinMode(_in2Pin, OUTPUT);
    // }
}

void L298N::forward()
{
    digitalWrite(_in1Pin, HIGH);
    digitalWrite(_in2Pin, LOW);
    analogWrite(_enPin, _speed);
    on = true;
    direction = FORWARD;
}

void L298N::backward()
{
    digitalWrite(_in1Pin, LOW);
    digitalWrite(_in2Pin, HIGH);
    analogWrite(_enPin, _speed);
    on = true;
    direction = BACKWARD;
}

void L298N::move()
{
    if (direction == FORWARD)
    {
        forward();
    }

    if (direction == BACKWARD)
    {
        backward();
    }
}

void L298N::stop()
{
    digitalWrite(_in1Pin, LOW);
    digitalWrite(_in2Pin, LOW);
    on = false;
}

int mapSpeed(int speed)
{
    return map(speed > 100 ? 100 : speed, 0, 100, 0, 255);
}

void L298N::setSpeed(int speed)
{
    _speed = mapSpeed(speed);
    analogWrite(_enPin, _speed);
}

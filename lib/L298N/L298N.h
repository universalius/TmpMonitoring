#ifndef L298N_H
#define L298N_H

#include <Arduino.h>

enum Direction
{
    FORWARD,
    BACKWARD,
};

class L298N
{
public:
    L298N(int enPin, int in1Pin, int in2Pin);

    bool on;
    Direction direction;

    void move();
    void forward();
    void backward();
    void stop();
    void setSpeed(int speed);

private:
    int _enPin;
    int _in1Pin;
    int _in2Pin;
    int _speed;
};

#endif // L298N_H
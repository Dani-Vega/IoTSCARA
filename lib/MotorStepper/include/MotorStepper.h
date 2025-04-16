#ifndef MOTORSTEPPER_H
#define MOTORSTEPPER_H

#include "driver/gpio.h"
#include <esp_timer.h>

#include <algorithm>
#include <cmath>

// Define Directions
#define DIRECTION_CW 1
#define DIRECTION_CCW 0

// Define interface types
#define FUNCTION 0
#define DRIVER 1
#define FULL2WIRE 2
#define FULL3WIRE 3
#define FULL4WIRE 4
#define HALF3WIRE 5
#define HALF4WIRE 6

#define HIGH 1
#define LOW 0

class MotorStepper
{
public:
    // Constructor for standard motor stepper setup
    MotorStepper(uint8_t interface, uint8_t pin1, uint8_t pin2, uint8_t pin3 = -1, uint8_t pin4 = -1, bool enable = true);
    
    // Constructor for functional motor stepper with forward and backward functions
    MotorStepper(void (*forward)(), void (*backward)());

    ~MotorStepper();

    // Motor control functions
    void moveToAbsolute(int absolute);
    void moveTo(int absolute);
    void move(int relative);
    bool runSpeed();
    void setCurrentPosition(int position);
    void stop();
    bool isRunning();
    bool runSpeedToPosition();
    
    // Getters for motor position
    int distanceToGo();
    int targetPosition();
    int currentPosition();
    
    // Setters for motor parameters
    void setMaxSpeed(float speed);
    float maxSpeed();
    void setAcceleration(float acceleration);
    float acceleration();
    void setSpeed(float speed);
    float speed();
    
    // Stepper movement functions
    void step(int step);
    int stepForward();
    int stepBackward();
    void setMinPulseWidth(unsigned int minWidth);
    void setEnablePin(uint8_t enablePin);
    void setPinsInverted(bool directionInvert, bool stepInvert, bool enableInvert);
    void setPinsInverted(bool pin1Invert, bool pin2Invert, bool pin3Invert, bool pin4Invert, bool enableInvert);
    
    // Motor status functions
    void enableOutputs();
    void disableOutputs();
    
    // Motor speed and acceleration calculation
    unsigned int computeNewSpeed();

    bool run();

private:
    // Pin and motor state variables
    uint8_t _interface;
    uint8_t _pin[4];
    uint8_t _enablePin;
    bool _enableInverted;
    uint8_t _pinInverted[4];
    
    int _currentPos;
    int _targetPos;
    int _n;
    int _c0, _cn, _cmin;
    int _direction;
    
    float _speed;
    float _maxSpeed;
    float _acceleration;
    float _sqrt_twoa;
    uint64_t _lastStepTime;
    uint32_t _stepInterval;
    uint32_t _minPulseWidth;

    void setOutputPins(uint8_t mask);

    void step0(int step);
    void step1(int step);
    void step2(int step);
    void step3(int step);
    void step4(int step);
    void step5(int step);
    void step6(int step);
    void step8(int step);

    // Functional interface variables
    void (*_forward)();
    void (*_backward)();
};

#define MULTISTEPPER_MAX_STEPPERS 10

class MultiStepper
{
public:
    /// Constructor
    MultiStepper();

    bool addStepper(MotorStepper& stepper);

    void moveTo(int absolute[]);
    
    bool run();
    
private:
    MotorStepper* _steppers[MULTISTEPPER_MAX_STEPPERS];

    uint8_t _num_steppers;
};


#endif

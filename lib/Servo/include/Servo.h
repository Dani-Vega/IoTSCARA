#ifndef SERVO_H
#define SERVO_H

#include "SimplePWM.h"

class Servo {
private:
    SimplePWM servoPWM;

    float mapValue(float x, float in_min, float in_max, float out_min, float out_max);
    
    float minDutyCycle;
    float maxDutyCycle;
    float minAngle;
    float maxAngle;

public:
    Servo();
    ~Servo();

    void setup(uint8_t gpio_num, uint8_t channel, uint8_t frecuency, float minDuty = 3.0f, float maxDuty = 13.0f, float minAngle = 0.0f, float maxAngle = 180.0f);
    void setServoAngle(float angle);
};

#endif // SERVO_H

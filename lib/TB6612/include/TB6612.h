#ifndef _PUENTEH_H
#define _PUENTEH_H

#include "SimplePWM.h"

class TB6612
{
public:
    TB6612();
    ~TB6612();
    void setup(uint8_t pins_pwm[], uint8_t pwm_channels[], uint32_t frecuencia, uint8_t resolution_bits, float max_voltage, float minimum_voltage, bool inverse = 0);
    void setSpeed(float speed_percentage);
    void setStop(bool brake);
private:
    float _speed_percentage;
    float _minimum_voltage;
    float _max_voltage;
    bool _inverse;
    uint8_t _pwm_channels[2];
    SimplePWM pwm[2];
};

#endif// _PUENTE_H
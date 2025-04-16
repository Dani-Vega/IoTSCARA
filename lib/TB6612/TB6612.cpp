#include "TB6612.h"

TB6612::TB6612() : _speed_percentage(0), _minimum_voltage(0.35f), _max_voltage(3.3f)
{
}

void TB6612::setup(uint8_t pins_pwm[], uint8_t pwm_channels[], uint32_t frecuencia, uint8_t resolution_bits, float max_voltage, float minimum_voltage, bool inverse)
{

    pwm[0].setup(pins_pwm[0], pwm_channels[0], 0, resolution_bits, frecuencia, LEDC_TIMER_0, LEDC_LOW_SPEED_MODE, LEDC_INTR_DISABLE);
    pwm[1].setup(pins_pwm[1], pwm_channels[1], 0, resolution_bits, frecuencia, LEDC_TIMER_0, LEDC_LOW_SPEED_MODE, LEDC_INTR_DISABLE);

    _minimum_voltage = minimum_voltage;
    _max_voltage = max_voltage;
    _inverse = inverse;
}

void TB6612::setSpeed(float speed_percentage)
{
    int8_t direccion;

    if (speed_percentage > 0)
    {
        direccion = 1;
    }
    else if (speed_percentage < 0)
    {
        direccion = -1;
        speed_percentage = -speed_percentage;
    }
    else
    {
        direccion = 0;
    }

    if (speed_percentage < 1)
    {
        speed_percentage = 1;
    }
    else if (speed_percentage > 100)
    {
        speed_percentage = 100;
    }

    float norm_voltage = _minimum_voltage + ((_max_voltage - _minimum_voltage) * (speed_percentage - 1) / 99.0f);
    _speed_percentage = static_cast<uint32_t>((norm_voltage / _max_voltage) * 100);

    direccion = _inverse ? -direccion : direccion;

    if (direccion > 0)
    {
        pwm[0].setDuty(_speed_percentage);
        pwm[1].setDuty(0);
    }
    else if (direccion < 0)
    {
        pwm[0].setDuty(0);
        pwm[1].setDuty(_speed_percentage);
    }
    else
    {
        setStop(false);
        return;
    }
}

void TB6612::setStop(bool brake)
{
    if (brake)
    {
        pwm[0].setDuty(1);
        pwm[1].setDuty(1);
    }
    else
    {
        pwm[0].setDuty(0);
        pwm[1].setDuty(0);
    }
}
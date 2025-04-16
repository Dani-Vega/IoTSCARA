#ifndef _SIMPLEPWM_H
#define _SIMPLEPWM_H

#include "driver/ledc.h"

class SimplePWM
{
public:
    SimplePWM();
    void setup(int gpio_num, uint8_t channel, unsigned int invert = 0, uint8_t bit_resolution = LEDC_TIMER_8_BIT, uint32_t frequency = 25000, ledc_timer_t timer = LEDC_TIMER_0, ledc_mode_t mode = LEDC_LOW_SPEED_MODE, ledc_intr_type_t interrupt = LEDC_INTR_DISABLE);
    void setDigitalLevel(uint32_t digital_level, uint32_t hpoint = 0);
    void setDuty(float duty);
    void setFrequency(uint32_t frequency);
    float getDuty();
    uint32_t getFrequency();
    
private:
    uint32_t _max_digital_level;
    ledc_timer_config_t ledc_timer;
    ledc_channel_t _channel;
    ledc_timer_t _timer;
    ledc_mode_t _mode;
};

#endif // _SIMPLEPWM_H
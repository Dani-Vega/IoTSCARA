#ifndef __RGB_H__
#define __RGB_H__

#include <SimplePWM.h>
#include <string>

class RGB
{
private:
    SimplePWM pwm[3];

public:
    RGB();
    ~RGB();
    void setup(uint8_t pins[3], uint8_t channels[3], unsigned int invert = 0, ledc_timer_t timer = LEDC_TIMER_0);
    void setColor (uint8_t r, uint8_t g, uint8_t b);
    void setColor (uint32_t rgb_code);
    void setColor(std::string hex_code);
};

#endif // __RGB_H__
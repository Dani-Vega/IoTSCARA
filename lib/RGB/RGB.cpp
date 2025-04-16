#include "RGB.h"

RGB::RGB()
{
}

RGB::~RGB()
{
}

void RGB::setup(uint8_t pins[3], uint8_t channels[3], unsigned int invert, ledc_timer_t timer)
{
    for (uint8_t i = 0; i < 3; i++)
        pwm[i].setup(pins[i], channels[i], invert, 8, 25000, timer);
}

void RGB::setColor(uint8_t r, uint8_t g, uint8_t b)
{
    pwm[0].setDigitalLevel(r);
    pwm[1].setDigitalLevel(g);
    pwm[2].setDigitalLevel(b);
}

void RGB::setColor(uint32_t rgb_code)
{
    setColor(rgb_code >> 16 & 0xFF, rgb_code >> 8 & 0xFF, rgb_code & 0xFF);
}

void RGB::setColor(std::string hex_code)
{
    if (hex_code.empty())
        hex_code = "000000";
    else if (hex_code[0] == '#')
        hex_code.erase(0, 1);

    uint32_t rgb_code = std::stoul(hex_code, nullptr, 16); // Convertir de string a uint32_t
    setColor(rgb_code);
}

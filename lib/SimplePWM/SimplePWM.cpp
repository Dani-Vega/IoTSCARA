#include "SimplePWM.h"

SimplePWM::SimplePWM()
{
}

void SimplePWM::setup(int gpio_num, uint8_t channel, unsigned int invert, uint8_t bit_resolution, uint32_t frequency, ledc_timer_t timer, ledc_mode_t mode, ledc_intr_type_t interrupt)
{
    _channel = (ledc_channel_t) channel;
    _timer = timer;
    _mode = mode;
    _max_digital_level = (1 << bit_resolution) - 1;
    ledc_timer_config_t ledc_timer = {
        .speed_mode = _mode,
        .duty_resolution = (ledc_timer_bit_t) bit_resolution,
        .timer_num = _timer,
        .freq_hz = frequency,
        .clk_cfg = LEDC_AUTO_CLK,
        .deconfigure = false,
    };
    ledc_timer_config(&ledc_timer);
    ledc_channel_config_t ledc_channel = {
        .gpio_num = gpio_num,
        .speed_mode = _mode,
        .channel = _channel,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = _timer,
        .duty = 0,
        .hpoint = 0,
        .flags = invert,
    };
    // ledc_channel.flags.output_invert = invert;
    ledc_channel_config(&ledc_channel);
    ledc_fade_func_install(0);
}

void SimplePWM::setDigitalLevel(uint32_t digital_level, uint32_t hpoint)
{
    if (digital_level > _max_digital_level)
        digital_level = _max_digital_level;
    ledc_set_duty_and_update(_mode, _channel, digital_level, hpoint);
}
void SimplePWM::setDuty(float duty_percentage)
{
    uint32_t digital_level = (uint32_t)(duty_percentage * _max_digital_level / 100.0f);
    setDigitalLevel(digital_level, 0);
}

void SimplePWM::setFrequency(uint32_t frequency)
{
    ledc_set_freq(_mode, _timer, frequency);
}
float SimplePWM::getDuty()
{
    return 100.0f * ledc_get_duty(_mode, _channel) / (float)_max_digital_level;
}

uint32_t SimplePWM::getFrequency()
{
    return ledc_get_freq(_mode,_timer);
}

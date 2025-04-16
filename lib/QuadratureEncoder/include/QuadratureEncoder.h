#ifndef __QUADRATUREENCODER_H__
#define __QUADRATUREENCODER_H__

#include <driver/gpio.h>
#include <esp_timer.h>
#include <esp_attr.h>

#include "Filter.h"

class QuadratureEncoder {
public:
    QuadratureEncoder();
    ~QuadratureEncoder();
    void setup(uint8_t gpio_num[], float degrees_per_edge, bool inverse, int64_t timeout_us = 100000);
    float getAngle();
    float getCumulativeAngle();
    float getSpeed();
    int8_t getDirection();
    void setAngle(float angle);

private:
    void IRAM_ATTR handler();
    gpio_num_t _gpio_num[2];
    float _degrees_per_edge;
    bool _inverse;
    volatile int64_t _counts, _timeout_us;
    volatile float _speed;
    volatile int8_t _direction;
    volatile int64_t _prev_micros, _delta_micros;
    volatile uint8_t _states;
    const int8_t _edge_lut[16] = {0, -1, 1, 0, 1, 0, 0, -1, -1, 0, 0, 1, 0, 1, -1, 0};

    // Añade el filtro pasa-bajas
    Filter speedFilter;
    float MA_b[4] = {0.25, 0.25, 0.25, 0.25};
    float MA_a[1] = {1.0};
};

#endif // __QUADRATUREENCODER_H__

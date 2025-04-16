#include "QuadratureEncoder.h"
#include "esp_log.h"

QuadratureEncoder::QuadratureEncoder() : _counts(0), _speed(0.0), _direction(0), _prev_micros(0), _delta_micros(0)
{
}

QuadratureEncoder::~QuadratureEncoder()
{
}

void QuadratureEncoder::setup(uint8_t gpio_num[], float degrees_per_edge, bool inverse, int64_t timeout_us)
{
    _gpio_num[0] = (gpio_num_t)gpio_num[0];
    _gpio_num[1] = (gpio_num_t)gpio_num[1];
    _degrees_per_edge = degrees_per_edge;
    _inverse = inverse;
    _timeout_us = timeout_us;

    // Configuración de pines e interrupciones como ya tienes
    gpio_config_t io_conf;
    io_conf.intr_type = GPIO_INTR_ANYEDGE;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pin_bit_mask = (1ULL << _gpio_num[0]) | (1ULL << _gpio_num[1]);
    gpio_config(&io_conf);

    static bool isr_service_installed = false;
    if (!isr_service_installed)
    {
        gpio_install_isr_service(ESP_INTR_FLAG_IRAM);
        isr_service_installed = true;
    }

    gpio_isr_handler_add(_gpio_num[0], [](void *arg)
                         { static_cast<QuadratureEncoder *>(arg)->handler(); }, this);
    gpio_isr_handler_add(_gpio_num[1], [](void *arg)
                         { static_cast<QuadratureEncoder *>(arg)->handler(); }, this);

    // Inicializa el filtro con los coeficientes del filtro (MA en este caso)
    speedFilter.setup(MA_b, MA_a, sizeof(MA_b) / sizeof(MA_b[0]), sizeof(MA_a) / sizeof(MA_a[0]));
}

float QuadratureEncoder::getAngle()
{
    // Calcula el ángulo actual basado en los counts
    float angle = _counts * _degrees_per_edge;

    // Ajusta el ángulo para que siempre esté entre 0 y 360 grados
    while (angle >= 360.0f)
    {
        angle -= 360.0f;
    }

    while (angle < 0.0f)
    {
        angle += 360.0f;
    }

    angle = _inverse ? -angle : angle;

    return angle;
}

float QuadratureEncoder::getCumulativeAngle()
{
    // Calcula el ángulo actual basado en los counts
    float angle = _counts * _degrees_per_edge;
    angle = _inverse ? -angle : angle;
    return angle;
}

float QuadratureEncoder::getSpeed()
{
    int64_t current_micros = esp_timer_get_time();
    if (current_micros - _prev_micros < _timeout_us)
        _speed = (_direction * (1000000.0 * _degrees_per_edge) / _delta_micros) / 6;
    else
        _speed = 0.0f;

    // Aplica el filtro pasa-bajas a la velocidad
    _speed = speedFilter.apply(_speed);
    return _inverse ? -_speed : _speed;
}

int8_t QuadratureEncoder::getDirection()
{
    return _direction;
}

void QuadratureEncoder::setAngle(float angle)
{
    _counts = angle / _degrees_per_edge;
}

void IRAM_ATTR QuadratureEncoder::handler()
{
    int64_t current_micros = esp_timer_get_time();
    _delta_micros = current_micros - _prev_micros;
    _prev_micros = current_micros;

    // Actualiza el estado de los pines
    _states = (_states << 2) & 0x0F; // Limita los 4 bits más bajos
    for (uint8_t i = 0; i < 2; i++)
    {
        _states |= (gpio_get_level(_gpio_num[i]) << i);
    }

    _direction = _edge_lut[_states]; // Determina la dirección con la LUT
    _counts += _direction;           // Actualiza los recuentos
}

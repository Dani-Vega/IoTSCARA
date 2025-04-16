#include "Servo.h"

Servo::Servo() {}

Servo::~Servo() {}

void Servo::setup(uint8_t gpio_num, uint8_t channel, uint8_t frecuency, float minDuty, float maxDuty, float minAngle, float maxAngle) {
    this->minDutyCycle = minDuty;
    this->maxDutyCycle = maxDuty;
    this->minAngle = minAngle;
    this->maxAngle = maxAngle;

    // Configurar el PWM con parámetros predeterminados para servomotores
    servoPWM.setup(gpio_num, channel, 0, 8, frecuency, LEDC_TIMER_0, LEDC_LOW_SPEED_MODE);
}

void Servo::setServoAngle(float angle) {
    // Limitar el ángulo dentro del rango permitido
    if (angle < minAngle) angle = minAngle;
    if (angle > maxAngle) angle = maxAngle;

    // Mapear el ángulo al ciclo de trabajo correspondiente
    float dutyCycle = mapValue(angle, minAngle, maxAngle, minDutyCycle, maxDutyCycle);
    servoPWM.setDuty(dutyCycle);
}

float Servo::mapValue(float x, float in_min, float in_max, float out_min, float out_max) {
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

#include "PID.h"

// Constructor de la clase PID
PID::PID()
    : _dt(0), _integral(0), _saturation(0) {
    _gains[0] = 0;
    _gains[1] = 0;
    _gains[2] = 0;
    _error[0] = 0;
    _error[1] = 0;
    _error[2] = 0;
}

// Método para configurar las ganancias, dt, y la saturación integral
void PID::setup(float gains[], float dt, float integral_saturation) {
    _dt = dt;
    _saturation = integral_saturation;
    setGains(gains);
}

// Método para establecer las ganancias del PID
void PID::setGains(float gains[]) {
    for (unsigned char i = 0; i < 3; i++) {
        _gains[i] = gains[i];
    }
}

// Método para calcular la salida del PID
float PID::calculate(float measurement, float reference) {
    _error[0] = reference - measurement;  // Error actual
    float control_output = _gains[0] * _error[0];  // Parte proporcional

    // Parte integral si ki > 0
    if (_gains[1] > 0.0001f) {
        _integral += (_dt / 2.0f) * (_error[0] + _error[1]);
        _integral = antiwindup(_integral);  // Anti-windup
        control_output += _gains[1] * _integral;  // Añadir parte integral
    }

    // Parte derivativa si kd > 0
    if (_gains[2] > 0.0001f) {
        control_output += _gains[2] * ((_error[0] - _error[2]) / (2 * _dt));
    }

    // Desplazar errores anteriores
    for (unsigned char i = 2; i > 0; i--) {
        _error[i] = _error[i - 1];
    }

    // Retornar la salida del control
    return control_output;
}

// Método para calcular la salida del PID
float PID::calculate(float error) {
    _error[0] = error;  // Error actual
    float control_output = _gains[0] * _error[0];  // Parte proporcional

    // Parte integral si ki > 0
    if (_gains[1] > 0.0001f) {
        _integral += (_dt / 2.0f) * (_error[0] + _error[1]);
        _integral = antiwindup(_integral);  // Anti-windup
        control_output += _gains[1] * _integral;  // Añadir parte integral
    }

    // Parte derivativa si kd > 0
    if (_gains[2] > 0.0001f) {
        control_output += _gains[2] * ((_error[0] - _error[2]) / (2 * _dt));
    }

    // Desplazar errores anteriores
    for (unsigned char i = 2; i > 0; i--) {
        _error[i] = _error[i - 1];
    }

    // Retornar la salida del control
    return control_output;
}

// Función anti-windup para limitar el valor de la parte integral
float PID::antiwindup(float integral) {
    if (integral > _saturation) {
        return _saturation;
    } else if (integral < -_saturation) {
        return -_saturation;
    }
    return integral;
}

// Método para obtener el error actual
float PID::getError() {
    return _error[0];
}

// Método para resetear la parte integral
void PID::resetIntegral() {
    _integral = 0;
}

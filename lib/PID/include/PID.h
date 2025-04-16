#ifndef PID_H
#define PID_H

class PID {
private:
    float _gains[3];       // Array para almacenar kp, ki, kd
    float _error[3];       // Array para almacenar errores anteriores
    float _dt;             // Tiempo de muestreo
    float _integral;       // Componente integral acumulada
    float _saturation;     // Saturación de la parte integral

    // Función para limitar el valor de la integral (anti-windup)
    float antiwindup(float integral);

public:
    // PID(float output_min, float output_max);
    PID();

    void setup(float gains[], float dt, float integral_saturation);
    // float calculate(float measurement, float reference);
    float calculate(float measurement, float reference);
    float calculate(float error);
    float getError();
    void setGains(float gains[]);
    void resetIntegral();
};

#endif // PID_H

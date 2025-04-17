#ifndef MOTORCONTROLLER_H
#define MOTORCONTROLLER_H

#include "TB6612.h"
#include "QuadratureEncoder.h"
#include "PID.h"

#include <math.h>

class MotorController
{
public:
    // Constructor
    MotorController(TB6612 &motor, QuadratureEncoder &encoder, PID &pid);

    // Configuración del controlador
    void setup(uint8_t pins_pwm[], uint8_t pwm_channels[], uint32_t frecuencia, uint8_t resolution_bits, float max_voltage, float minimum_voltage, uint8_t encoder_pins[], float degrees_per_edge, int64_t timeout_us, float dt, float integral_saturation, bool inverse = 0);

    // Actualizar el control del motor
    void update();

    // Establecer el modo de control (velocidad, posición, sin control)
    void setControlMode(int control_mode);

    // Establecer una nueva referencia (velocidad o posición)
    void setReference(float new_reference);
    void move(float delta);
    void moveTo(float new_reference);

    // Establecer las ganancias del PID
    void setPIDGains(float gains[]);

    // Obtener el error actual del PID
    float getError();

    // Obtener la velocidad actual del encoder
    float getSpeed();

    // Obtener la salida calculada del PID (u)
    float getOutput();

    //Movimiento relativo basado en la referencia actual
    float getReference();

    // Detener el puente H
    void stop(bool brake);

    // Método para resetear la parte integral
    void resetIntegral();

    bool isRunning();

private:
    TB6612 &motor;              // Motor controlado
    QuadratureEncoder &encoder; // Encoder para medir la velocidad/posición
    PID &pid;                   // Controlador PID para el motor

    float _pid_gains[3]; // Ganancias P, I, D
    float _reference;    // Valor de referencia (velocidad o posición)
    int _mode;           // Modo de control (velocidad, posición, sin control)
    float _speed;        // Velocidad calculada del encoder
    float _position;     // Posición angular calculada del encoder
    float u;             // Salida del controlador PID

    enum ControlMode
    {
        NO_CONTROL,
        SPEED,
        POSITION
    };
};

#define MAX_MOTORS 4

class MultiDC
{
public:
    MultiDC();
    ~MultiDC();

    bool addMotor(MotorController &motor);
    void update(); // Llama a update() de todos los motores
    void setControlMode(int control_mode);
    void setReferences(float references[]);
    void stopAll(bool brake = true);
    int motorCount();

private:
    MotorController *_motors[MAX_MOTORS];
    int _num_motors;
};

#endif // MOTORCONTROLLER_H

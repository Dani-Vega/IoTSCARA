#include "MotorController.h"

MotorController::MotorController(TB6612 &motor, QuadratureEncoder &encoder, PID &pid)
    : motor(motor), encoder(encoder), pid(pid), _reference(0.0f), _mode(0), u(0.0f)
{
    // Inicializar las ganancias del PID en 0
    _pid_gains[0] = _pid_gains[1] = _pid_gains[2] = 0.0f;
}

void MotorController::setup(uint8_t pins_pwm[], uint8_t pwm_channels[], uint32_t frecuencia, uint8_t resolution_bits, float max_voltage, float minimum_voltage, uint8_t encoder_pins[], float degrees_per_edge, int64_t timeout_us, float dt, float integral_saturation, bool inverse)
{
    motor.setup(pins_pwm, pwm_channels, frecuencia, resolution_bits, max_voltage, minimum_voltage, inverse);
    encoder.setup(encoder_pins, degrees_per_edge, timeout_us, inverse);
    pid.setup(_pid_gains, dt, integral_saturation);
}

void MotorController::update()
{
    _speed = encoder.getSpeed();
    _position = encoder.getCumulativeAngle();
    // Controlar motor según el modo seleccionado
    switch (_mode)
    {
    case NO_CONTROL:
        u = _reference;
        break;
    case SPEED:
        u = pid.calculate(_speed, _reference);
        break;
    case POSITION:
        u = pid.calculate(_position, _reference);
        break;
    }
    motor.setSpeed(u);
}

void MotorController::setControlMode(int control_mode)
{
    _mode = control_mode;
}

void MotorController::setReference(float new_reference)
{
    _reference = new_reference;
}

void MotorController::move(float delta) // For position (Relative)
{
    _reference += delta;
}

void MotorController::moveTo(float target) // For position (Absolute)
{
    _reference = target;
}

void MotorController::setPIDGains(float gains[])
{
    for (int i = 0; i < 3; i++)
    {
        _pid_gains[i] = gains[i];
    }
    pid.setGains(_pid_gains);
}

// Métodos para obtener los valores de error, velocidad y salida PID

float MotorController::getError()
{
    return pid.getError(); // Devuelve el error actual del PID
}

float MotorController::getSpeed()
{
    return _speed; // Devuelve la velocidad calculada
}

float MotorController::getOutput()
{
    return u; // Devuelve la salida calculada del PID (u)
}

void MotorController::stop(bool brake)
{
    motor.setStop(brake);
}

// Método para resetear la parte integral
void MotorController::resetIntegral()
{
    pid.resetIntegral();
}

bool MotorController::isRunning()
{
    if (fabs(_reference - encoder.getCumulativeAngle()) > 0.9)
    {
        return true; // El motor sigue en movimiento
    }

    return false; // El motor está detenido
}

MultiDC::MultiDC() : _num_motors(0) {}

MultiDC::~MultiDC() {}

bool MultiDC::addMotor(MotorController &motor)
{
    if (_num_motors >= MAX_MOTORS)
        return false;
    _motors[_num_motors++] = &motor;
    return true;
}

void MultiDC::update()
{
    for (int i = 0; i < _num_motors; ++i)
    {
        if (_motors[i]->isRunning())
        {
            _motors[i]->update();
        }
    }
}

void MultiDC::setControlMode(int control_mode)
{
    for (int i = 0; i < _num_motors; ++i)
    {
        _motors[i]->setControlMode(control_mode);
    }
}

void MultiDC::setReferences(float references[])
{
    for (int i = 0; i < _num_motors; ++i)
    {
        _motors[i]->setReference(references[i]);
    }
}

void MultiDC::stopAll(bool brake)
{
    for (int i = 0; i < _num_motors; ++i)
    {
        _motors[i]->stop(brake);
    }
}

int MultiDC::motorCount()
{
    return _num_motors;
}

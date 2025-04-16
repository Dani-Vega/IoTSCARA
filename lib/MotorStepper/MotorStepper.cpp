#include "MotorStepper.h"

template <typename T>
T max(T a, T b) {
    return (a > b) ? a : b;
}

template <typename T>
T constrain(T x, T a, T b) {
    if (x < a) return a;
    if (x > b) return b;
    return x;
}

MotorStepper::MotorStepper(uint8_t interface, uint8_t pin1, uint8_t pin2, uint8_t pin3, uint8_t pin4, bool enable)
{
    _interface = interface;
    _currentPos = 0;
    _targetPos = 0;
    _speed = 0.0;
    _maxSpeed = 0.0;
    _acceleration = 0.0;
    _sqrt_twoa = 1.0;
    _stepInterval = 0;
    _minPulseWidth = 1;
    _enablePin = 0xff;
    _lastStepTime = 0;
    _pin[0] = pin1;
    _pin[1] = pin2;
    _pin[2] = pin3;
    _pin[3] = pin4;
    _enableInverted = false;

    // NEW
    _n = 0;
    _c0 = 0.0;
    _cn = 0.0;
    _cmin = 1.0;
    _direction = DIRECTION_CCW;

    int i;
    for (i = 0; i < 4; i++)
        _pinInverted[i] = 0;
    if (enable)
        enableOutputs();
    // Some reasonable default
    setAcceleration(1);
    setMaxSpeed(1);
}

MotorStepper::MotorStepper(void (*forward)(), void (*backward)())
{
    _interface = 0;
    _currentPos = 0;
    _targetPos = 0;
    _speed = 0.0;
    _maxSpeed = 0.0;
    _acceleration = 0.0;
    _sqrt_twoa = 1.0;
    _stepInterval = 0;
    _minPulseWidth = 1;
    _enablePin = 0xff;
    _lastStepTime = 0;
    _pin[0] = 0;
    _pin[1] = 0;
    _pin[2] = 0;
    _pin[3] = 0;
    _forward = forward;
    _backward = backward;

    // NEW
    _n = 0;
    _c0 = 0.0;
    _cn = 0.0;
    _cmin = 1.0;
    _direction = DIRECTION_CCW;

    int i;
    for (i = 0; i < 4; i++)
        _pinInverted[i] = 0;
    // Some reasonable default
    setAcceleration(1);
    setMaxSpeed(1);
}

MotorStepper::~MotorStepper(){};

void MotorStepper::moveTo(int absolute)
{
    if (_targetPos != absolute)
    {
        _targetPos = absolute;
        computeNewSpeed();
    }
}

void MotorStepper::move(int relative)
{
    moveTo(_currentPos + relative);
}

void MotorStepper::moveToAbsolute(int absolute)
{
    // Calcula el desplazamiento relativo a la posición actual
    int relativeMovement = absolute - _currentPos;
    move(relativeMovement);  // Mueve el motor al nuevo valor absoluto usando el desplazamiento relativo
}

// Executes a single step if the time interval has elapsed since the last step.
// Must call this function periodically in the main loop to step the motor at the correct intervals.
// Returns true if the motor has moved (a step was made), false if no step occurred.
bool MotorStepper::runSpeed()
{
    // Do not do anything unless we actually have a step interval
    if (!_stepInterval)
        return false;

    uint64_t time = esp_timer_get_time();
    if (time - _lastStepTime >= _stepInterval)
    {
        if (_direction == DIRECTION_CW)
        {
            // Clockwise
            _currentPos += 1;
        }
        else
        {
            // Counterclockwise
            _currentPos -= 1;
        }
        step(_currentPos);

        _lastStepTime = time;

        return true;
    }
    else
    {
        return false;
    }
}

int MotorStepper::distanceToGo()
{
    return _targetPos - _currentPos;
}

int MotorStepper::targetPosition()
{
    return _targetPos;
}

int MotorStepper::currentPosition()
{
    return _currentPos;
}

// Resets the motor's position to the specified value.
// This function is useful during initialization or after the motor has moved to its initial position.
// It also resets the speed, step interval, and internal counters to prepare the motor for further movement.
void MotorStepper::setCurrentPosition(int position)
{
    _targetPos = _currentPos = position;
    _n = 0;
    _stepInterval = 0;
    _speed = 0.0;
}

// Subclasses can override
unsigned int MotorStepper::computeNewSpeed()
{
    int distanceTo = distanceToGo(); // +ve is clockwise from curent location

    int stepsToStop = (int)((_speed * _speed) / (2.0 * _acceleration)); // Equation 16

    if (distanceTo == 0 && stepsToStop <= 1)
    {
        // We are at the target and its time to stop
        _stepInterval = 0;
        _speed = 0.0;
        _n = 0;
        return _stepInterval;
    }

    if (distanceTo > 0)
    {
        // We are anticlockwise from the target
        // Need to go clockwise from here, maybe decelerate now
        if (_n > 0)
        {
            // Currently accelerating, need to decel now? Or maybe going the wrong way?
            if ((stepsToStop >= distanceTo) || _direction == DIRECTION_CCW)
                _n = -stepsToStop; // Start deceleration
        }
        else if (_n < 0)
        {
            // Currently decelerating, need to accel again?
            if ((stepsToStop < distanceTo) && _direction == DIRECTION_CW)
                _n = -_n; // Start accceleration
        }
    }
    else if (distanceTo < 0)
    {
        // We are clockwise from the target
        // Need to go anticlockwise from here, maybe decelerate
        if (_n > 0)
        {
            // Currently accelerating, need to decel now? Or maybe going the wrong way?
            if ((stepsToStop >= -distanceTo) || _direction == DIRECTION_CW)
                _n = -stepsToStop; // Start deceleration
        }
        else if (_n < 0)
        {
            // Currently decelerating, need to accel again?
            if ((stepsToStop < -distanceTo) && _direction == DIRECTION_CCW)
                _n = -_n; // Start accceleration
        }
    }

    // Need to accelerate or decelerate
    if (_n == 0)
    {
        // First step from stopped
        _cn = _c0;
        _direction = (distanceTo > 0) ? DIRECTION_CW : DIRECTION_CCW;
    }
    else
    {
        // Subsequent step. Works for accel (n is +_ve) and decel (n is -ve).
        _cn = _cn - ((2.0 * _cn) / ((4.0 * _n) + 1)); // Equation 13
        _cn = max(_cn, _cmin);
    }
    _n++;
    _stepInterval = _cn;
    _speed = 1000000.0 / _cn;
    if (_direction == DIRECTION_CCW)
        _speed = -_speed;

    return _stepInterval;
}

// Run the motor to implement speed and acceleration in order to proceed to the target position
// You must call this at least once per step, preferably in your main loop
// If the motor is in the desired position, the cost is very small
// returns true if the motor is still running to the target position.
bool MotorStepper::run()
{
    if (runSpeed())
        computeNewSpeed();
    return _speed != 0.0 || distanceToGo() != 0;
}

void MotorStepper::setMaxSpeed(float speed)
{
    if (speed < 0.0)
        speed = -speed;
    if (_maxSpeed != speed)
    {
        _maxSpeed = speed;
        _cmin = 1000000.0 / speed;
        // Recompute _n from current speed and adjust speed if accelerating or cruising
        if (_n > 0)
        {
            _n = (int)((_speed * _speed) / (2.0 * _acceleration)); // Equation 16
            computeNewSpeed();
        }
    }
}

float MotorStepper::maxSpeed()
{
    return _maxSpeed;
}

void MotorStepper::setAcceleration(float acceleration)
{
    if (acceleration == 0.0)
        return;
    if (acceleration < 0.0)
        acceleration = -acceleration;
    if (_acceleration != acceleration)
    {
        // Recompute _n per Equation 17
        _n = _n * (_acceleration / acceleration);
        // New c0 per Equation 7, with correction per Equation 15
        _c0 = 0.676 * sqrt(2.0 / acceleration) * 1000000.0; // Equation 15
        _acceleration = acceleration;
        computeNewSpeed();
    }
}

float MotorStepper::acceleration()
{
    return _acceleration;
}

void MotorStepper::setSpeed(float speed)
{
    if (speed == _speed)
        return;
    speed = constrain(speed, -_maxSpeed, _maxSpeed);
    if (speed == 0.0)
        _stepInterval = 0;
    else
    {
        _stepInterval = fabs(1000000.0 / speed);
        _direction = (speed > 0.0) ? DIRECTION_CW : DIRECTION_CCW;
    }
    _speed = speed;
}

float MotorStepper::speed()
{
    return _speed;
}

// Subclasses can override
void MotorStepper::step(int step)
{
    switch (_interface)
    {
    case FUNCTION:
        step0(step);
        break;

    case DRIVER:
        step1(step);
        break;

    case FULL2WIRE:
        step2(step);
        break;

    case FULL3WIRE:
        step3(step);
        break;

    case FULL4WIRE:
        step4(step);
        break;

    case HALF3WIRE:
        step6(step);
        break;

    case HALF4WIRE:
        step8(step);
        break;
    }
}

int MotorStepper::stepForward()
{
    // Clockwise
    _currentPos += 1;
    step(_currentPos);
    _lastStepTime = esp_timer_get_time(); 
    return _currentPos;
}


int MotorStepper::stepBackward()
{
    // Counter-clockwise
    _currentPos -= 1;
    step(_currentPos);
    _lastStepTime = esp_timer_get_time(); 
    return _currentPos;
}

// You might want to override this to implement eg serial output
// bit 0 of the mask corresponds to _pin[0]
// bit 1 of the mask corresponds to _pin[1]
// ....

void MotorStepper::setOutputPins(uint8_t mask)
{
    uint8_t numpins = 2;
    if (_interface == FULL4WIRE || _interface == HALF4WIRE)
        numpins = 4;
    else if (_interface == FULL3WIRE || _interface == HALF3WIRE)
        numpins = 3;
    
    // Configura los pines como salida si aún no lo están
    for (uint8_t i = 0; i < numpins; i++) {
        gpio_set_direction((gpio_num_t)_pin[i], GPIO_MODE_OUTPUT);
    }
    
    // Establecer el valor de los pines según la máscara
    for (uint8_t i = 0; i < numpins; i++) {
        if (mask & (1 << i)) {
            gpio_set_level((gpio_num_t)_pin[i], HIGH ^ _pinInverted[i]);
        } else {
            gpio_set_level((gpio_num_t)_pin[i], LOW ^ _pinInverted[i]);
        }
    }
}

// 0 pin step function (ie for functional usage)
void MotorStepper::step0(int step)
{
    (void)(step); // Unused
    if (_speed > 0)
        _forward();
    else
        _backward();
}

// 1 pin step function (ie for stepper drivers)
// This is passed the current step number (0 to 7)
// Subclasses can override
void MotorStepper::step1(int step)
{
    (void)(step);  // Unused

    // _pin[0] is step, _pin[1] is direction
    setOutputPins(_direction ? 0b10 : 0b00);  // Set direction first else get rogue pulses
    setOutputPins(_direction ? 0b11 : 0b01);  // step HIGH

    // Obtener el tiempo actual en microsegundos
    uint64_t current_time = esp_timer_get_time();

    // Guardamos el tiempo cuando el pulso fue enviado
    if (current_time - _lastStepTime >= _minPulseWidth) {
        // Step pulse time elapsed, set step LOW to complete the pulse cycle
        setOutputPins(_direction ? 0b10 : 0b00);  // step LOW
        _lastStepTime = current_time;  // Actualiza el último tiempo del paso
    }
}

// 2 pin step function
// This is passed the current step number (0 to 7)
// Subclasses can override
void MotorStepper::step2(int step)
{
    switch (step & 0x3)
    {
    case 0: /* 01 */
        setOutputPins(0b10);
        break;

    case 1: /* 11 */
        setOutputPins(0b11);
        break;

    case 2: /* 10 */
        setOutputPins(0b01);
        break;

    case 3: /* 00 */
        setOutputPins(0b00);
        break;
    }
}
// 3 pin step function
// This is passed the current step number (0 to 7)
// Subclasses can override
void MotorStepper::step3(int step)
{
    switch (step % 3)
    {
    case 0: // 100
        setOutputPins(0b100);
        break;

    case 1: // 001
        setOutputPins(0b001);
        break;

    case 2: // 010
        setOutputPins(0b010);
        break;
    }
}

// 4 pin step function for half stepper
// This is passed the current step number (0 to 7)
// Subclasses can override
void MotorStepper::step4(int step)
{
    switch (step & 0x3)
    {
    case 0: // 1010
        setOutputPins(0b0101);
        break;

    case 1: // 0110
        setOutputPins(0b0110);
        break;

    case 2: // 0101
        setOutputPins(0b1010);
        break;

    case 3: // 1001
        setOutputPins(0b1001);
        break;
    }
}

// 3 pin half step function
// This is passed the current step number (0 to 7)
// Subclasses can override
void MotorStepper::step6(int step)
{
    switch (step % 6)
    {
    case 0: // 100
        setOutputPins(0b100);
        break;

    case 1: // 101
        setOutputPins(0b101);
        break;

    case 2: // 001
        setOutputPins(0b001);
        break;

    case 3: // 011
        setOutputPins(0b011);
        break;

    case 4: // 010
        setOutputPins(0b010);
        break;

    case 5: // 011
        setOutputPins(0b110);
        break;
    }
}

// 4 pin half step function
// This is passed the current step number (0 to 7)
// Subclasses can override
void MotorStepper::step8(int step)
{
    switch (step & 0x7)
    {
    case 0: // 1000
        setOutputPins(0b0001);
        break;

    case 1: // 1010
        setOutputPins(0b0101);
        break;

    case 2: // 0010
        setOutputPins(0b0100);
        break;

    case 3: // 0110
        setOutputPins(0b0110);
        break;

    case 4: // 0100
        setOutputPins(0b0010);
        break;

    case 5: // 0101
        setOutputPins(0b1010);
        break;

    case 6: // 0001
        setOutputPins(0b1000);
        break;

    case 7: // 1001
        setOutputPins(0b1001);
        break;
    }
}

// Prevents power consumption on the outputs
void MotorStepper::disableOutputs()
{
    if (!_interface)
        return;

    setOutputPins(0);  // Controla los pines, poniéndolos en bajo (LOW) o apagados

    if (_enablePin != 0xff)
    {
        // Configurar el pin de habilitación como salida si no es 0xff (no deshabilitado)
        gpio_set_direction((gpio_num_t)_enablePin, GPIO_MODE_OUTPUT);
        // Establecer el nivel del pin de habilitación (invirtiendo si es necesario)
        gpio_set_level((gpio_num_t)_enablePin, LOW ^ _enableInverted);
    }
}

void MotorStepper::enableOutputs()
{
    if (!_interface)
        return;

    // Configura los pines de control como salidas
    gpio_set_direction((gpio_num_t)_pin[0], GPIO_MODE_OUTPUT); //Step pin
    gpio_set_direction((gpio_num_t)_pin[1], GPIO_MODE_OUTPUT); //Dir pin
    
    if (_interface == FULL4WIRE || _interface == HALF4WIRE)
    {
        gpio_set_direction((gpio_num_t)_pin[2], GPIO_MODE_OUTPUT);
        gpio_set_direction((gpio_num_t)_pin[3], GPIO_MODE_OUTPUT);
    }
    else if (_interface == FULL3WIRE || _interface == HALF3WIRE)
    {
        gpio_set_direction((gpio_num_t)_pin[2], GPIO_MODE_OUTPUT);
    }

    // Si el pin de habilitación está definido (no es 0xff), configúralo como salida
    if (_enablePin != 0xff)
    {
        gpio_set_direction((gpio_num_t)_enablePin, GPIO_MODE_OUTPUT);
        gpio_set_level((gpio_num_t)_enablePin, HIGH ^ _enableInverted);  // Establecer el nivel (HIGH o LOW dependiendo de la inversión)
    }
}

void MotorStepper::setMinPulseWidth(unsigned int minWidth)
{
    _minPulseWidth = minWidth;
}

void MotorStepper::setEnablePin(uint8_t enablePin)
{
    _enablePin = enablePin;

    // Esta acción ocurre después de la construcción, así que inicializa el pin ahora
    if (_enablePin != 0xff)
    {
        // Configura el pin de habilitación como salida
        gpio_set_direction((gpio_num_t)_enablePin, GPIO_MODE_OUTPUT);
        // Establece el nivel del pin (HIGH o LOW dependiendo de si está invertido)
        gpio_set_level((gpio_num_t)_enablePin, HIGH ^ _enableInverted);
    }
}

void MotorStepper::setPinsInverted(bool directionInvert, bool stepInvert, bool enableInvert)
{
    _pinInverted[0] = stepInvert;
    _pinInverted[1] = directionInvert;
    _enableInverted = enableInvert;
}

void MotorStepper::setPinsInverted(bool pin1Invert, bool pin2Invert, bool pin3Invert, bool pin4Invert, bool enableInvert)
{
    _pinInverted[0] = pin1Invert;
    _pinInverted[1] = pin2Invert;
    _pinInverted[2] = pin3Invert;
    _pinInverted[3] = pin4Invert;
    _enableInverted = enableInvert;
}

bool MotorStepper::runSpeedToPosition()
{
    if (_targetPos == _currentPos)
        return false;
    if (_targetPos > _currentPos)
        _direction = DIRECTION_CW;
    else
        _direction = DIRECTION_CCW;
    return runSpeed();
}

void MotorStepper::stop()
{
    if (_speed != 0.0)
    {
        int stepsToStop = (int)((_speed * _speed) / (2.0 * _acceleration)) + 1; // Equation 16 (+integer rounding)
        if (_speed > 0)
            move(stepsToStop);
        else
            move(-stepsToStop);
    }
}

bool MotorStepper::isRunning()
{
    return !(_speed == 0.0 && _targetPos == _currentPos);
}

//MultiStepper Class

MultiStepper::MultiStepper()
    : _num_steppers(0)
{
}

bool MultiStepper::addStepper(MotorStepper& stepper)
{
    if (_num_steppers >= MULTISTEPPER_MAX_STEPPERS)
	return false; // No room for more
    _steppers[_num_steppers++] = &stepper;
    return true;
}

void MultiStepper::moveTo(int absolute[])
{
    // First find the stepper that will take the longest time to move
    float longestTime = 0.0;

    uint8_t i;
    for (i = 0; i < _num_steppers; i++)
    {
	int thisDistance = absolute[i] - _steppers[i]->currentPosition();
	float thisTime = abs(thisDistance) / _steppers[i]->maxSpeed();

	if (thisTime > longestTime)
	    longestTime = thisTime;
    }

    if (longestTime > 0.0)
    {
	for (i = 0; i < _num_steppers; i++)
	{
	    int thisDistance = absolute[i] - _steppers[i]->currentPosition();
	    float thisSpeed = thisDistance / longestTime;
	    _steppers[i]->moveTo(absolute[i]); // New target position (resets speed)
	    _steppers[i]->setSpeed(thisSpeed); // New speed
	}
    }
}

// Returns true if any motor is still running to the target position.
bool MultiStepper::run()
{
    uint8_t i;
    bool ret = false;
    for (i = 0; i < _num_steppers; i++)
    {
	if ( _steppers[i]->distanceToGo() != 0)
	{
	    _steppers[i]->run();
	    ret = true;
	}
    }
    return ret;
}

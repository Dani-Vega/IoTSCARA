#include "AS5600.h"

//  CONFIGURATION REGISTERS
const uint8_t AS5600_ZMCO = 0x00;
const uint8_t AS5600_ZPOS = 0x01; //  + 0x02
const uint8_t AS5600_MPOS = 0x03; //  + 0x04
const uint8_t AS5600_MANG = 0x05; //  + 0x06
const uint8_t AS5600_CONF = 0x07; //  + 0x08

//  CONFIGURATION BIT MASKS - byte level
const uint8_t AS5600_CONF_POWER_MODE = 0x03;
const uint8_t AS5600_CONF_HYSTERESIS = 0x0C;
const uint8_t AS5600_CONF_OUTPUT_MODE = 0x30;
const uint8_t AS5600_CONF_PWM_FREQUENCY = 0xC0;
const uint8_t AS5600_CONF_SLOW_FILTER = 0x03;
const uint8_t AS5600_CONF_FAST_FILTER = 0x1C;
const uint8_t AS5600_CONF_WATCH_DOG = 0x20;

//  UNKNOWN REGISTERS 0x09-0x0A

//  OUTPUT REGISTERS
const uint8_t AS5600_RAW_ANGLE = 0x0C; //  + 0x0D
const uint8_t AS5600_ANGLE = 0x0E;     //  + 0x0F

// I2C_ADDRESS REGISTERS (AS5600L)
const uint8_t AS5600_I2CADDR = 0x20;
const uint8_t AS5600_I2CUPDT = 0x21;

//  STATUS REGISTERS
const uint8_t AS5600_STATUS = 0x0B;
const uint8_t AS5600_AGC = 0x1A;
const uint8_t AS5600_MAGNITUDE = 0x1B; //  + 0x1C
const uint8_t AS5600_BURN = 0xFF;

//  STATUS BITS
const uint8_t AS5600_MAGNET_HIGH = 0x08;
const uint8_t AS5600_MAGNET_LOW = 0x10;
const uint8_t AS5600_MAGNET_DETECT = 0x20;

AS5600::AS5600(i2c_port_t i2c_num, uint8_t address, uint32_t freq)
{
    _i2c_port = i2c_num;
    _address = address;
    _freq = freq;
}

AS5600::~AS5600()
{

}

bool AS5600::setup(uint8_t sdaPin, uint8_t sclPin, uint8_t directionPin)
{
    _sdaPin = sdaPin;
    _sclPin = sclPin;
    _directionPin = directionPin;

    if (_directionPin != AS5600_UNUSED_GPIO)
    {
        gpio_config_t io_conf = {
            .pin_bit_mask = 1ULL << _directionPin,
            .mode = GPIO_MODE_OUTPUT,
            .pull_up_en = GPIO_PULLUP_DISABLE,
            .pull_down_en = GPIO_PULLDOWN_DISABLE,
            .intr_type = GPIO_INTR_DISABLE};
        gpio_config(&io_conf);
    }

    setDirection(AS5600_CLOCK_WISE);

    i2c_config_t conf;
    conf.mode = I2C_MODE_MASTER;
    conf.clk_flags = 0;
    conf.sda_io_num = _sdaPin;
    conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
    conf.scl_io_num = _sclPin;
    conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
    conf.master.clk_speed = _freq;
    i2c_param_config(_i2c_port, &conf);
    i2c_driver_install(_i2c_port, I2C_MODE_MASTER, 0, 0, 0);

    return isConnected();
}

bool AS5600::isConnected()
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();

    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (_address << 1) | I2C_MASTER_WRITE, true);
    i2c_master_stop(cmd);

    esp_err_t err = i2c_master_cmd_begin(_i2c_port, cmd, pdMS_TO_TICKS(100));
    i2c_cmd_link_delete(cmd);

    return (err == ESP_OK);
}

uint8_t AS5600::getAddress()
{
    return _address;
}

/////////////////////////////////////////////////////////
//
//  CONFIGURATION REGISTERS + direction pin
//

void AS5600::setDirection(uint8_t direction)
{
    _direction = direction;

    if (_directionPin != AS5600_UNUSED_GPIO)
    {
        gpio_set_level((gpio_num_t)_directionPin, _direction);
    }
}

uint8_t AS5600::getDirection()
{
    if (_directionPin != AS5600_UNUSED_GPIO)
    {
        _direction = gpio_get_level((gpio_num_t)_directionPin);
    }
    return _direction;
}

uint8_t AS5600::getZMCO()
{
    uint8_t value = 0;
    if (readReg(AS5600_ZMCO, value) != ESP_OK)
        return 0;
    return value;
}

bool AS5600::setZPosition(uint16_t value)
{
    if (value > 0x0FFF)
        return false;
    writeReg2(AS5600_ZPOS, value);
    return true;
}

uint16_t AS5600::getZPosition()
{
    uint16_t value = 0;
    if (readReg2(AS5600_ZPOS, value) != ESP_OK)
    {
        return 0; // o algún valor reservado si quieres indicar error
    }
    return value & 0x0FFF;
}

bool AS5600::setMPosition(uint16_t value)
{
    if (value > 0x0FFF)
        return false;
    writeReg2(AS5600_MPOS, value);
    return true;
}

uint16_t AS5600::getMPosition()
{
    uint16_t value = 0;
    if (readReg2(AS5600_MPOS, value) != ESP_OK)
    {
        return 0; // o algún valor reservado como 0xFFFF si quieres detectar error
    }
    return value & 0x0FFF;
}

bool AS5600::setMaxAngle(uint16_t value)
{
    if (value > 0x0FFF)
        return false;
    writeReg2(AS5600_MANG, value);
    return true;
}

uint16_t AS5600::getMaxAngle()
{
    uint16_t value = 0;
    if (readReg2(AS5600_MANG, value) != ESP_OK)
    {
        return 0; // o se puede retornar 0xFFFF si se quiere indicar error
    }
    return value & 0x0FFF;
}

/////////////////////////////////////////////////////////
//
//  CONFIGURATION
//

bool AS5600::setConfigure(uint16_t value)
{
    if (value > 0x3FFF)
        return false;
    writeReg2(AS5600_CONF, value);
    return true;
}

uint16_t AS5600::getConfigure()
{
    uint16_t value = 0;
    if (readReg2(AS5600_CONF, value) != ESP_OK)
    {
        return 0;
    }
    return value & 0x3FFF;
}

//  details configure
bool AS5600::setPowerMode(uint8_t powerMode)
{
    if (powerMode > 3)
        return false;

    uint8_t value = 0;
    if (readReg(AS5600_CONF + 1, value) != ESP_OK)
    {
        return false;
    }

    value &= ~AS5600_CONF_POWER_MODE;              // limpiar bits de power mode
    value |= (powerMode & AS5600_CONF_POWER_MODE); // establecer nuevo modo

    if (writeReg(AS5600_CONF + 1, value) != ESP_OK)
    {
        return false;
    }

    return true;
}

uint8_t AS5600::getPowerMode()
{
    uint8_t value = 0;
    if (readReg(AS5600_CONF + 1, value) != ESP_OK)
    {
        return 0xFF; // valor reservado que indica error
    }

    return value & 0x03;
}

bool AS5600::setHysteresis(uint8_t hysteresis)
{
    if (hysteresis > 3)
        return false;

    uint8_t value = 0;
    if (readReg(AS5600_CONF + 1, value) != ESP_OK)
    {
        return false;
    }

    value &= ~AS5600_CONF_HYSTERESIS; // limpia bits 2 y 3
    value |= (hysteresis << 2);       // inserta hysteresis en bits 2 y 3

    if (writeReg(AS5600_CONF + 1, value) != ESP_OK)
    {
        return false;
    }

    return true;
}

uint8_t AS5600::getHysteresis()
{
    uint8_t value = 0;
    if (readReg(AS5600_CONF + 1, value) != ESP_OK)
    {
        return 0xFF; // valor reservado para indicar error
    }

    return (value >> 2) & 0x03;
}

bool AS5600::setOutputMode(uint8_t outputMode)
{
    if (outputMode > 2)
        return false;

    uint8_t value = 0;
    if (readReg(AS5600_CONF + 1, value) != ESP_OK)
    {
        return false;
    }

    value &= ~AS5600_CONF_OUTPUT_MODE; // limpia bits 4 y 5
    value |= (outputMode << 4);        // coloca nuevo modo en bits 4 y 5

    if (writeReg(AS5600_CONF + 1, value) != ESP_OK)
    {
        return false;
    }

    return true;
}

uint8_t AS5600::getOutputMode()
{
    uint8_t value = 0;
    if (readReg(AS5600_CONF + 1, value) != ESP_OK)
    {
        return 0xFF; // valor especial para indicar error
    }

    return (value >> 4) & 0x03;
}

bool AS5600::setPWMFrequency(uint8_t pwmFreq)
{
    if (pwmFreq > 3)
        return false;

    uint8_t value = 0;
    if (readReg(AS5600_CONF + 1, value) != ESP_OK)
    {
        return false;
    }

    value &= ~AS5600_CONF_PWM_FREQUENCY; // limpia bits 6 y 7
    value |= (pwmFreq << 6);             // coloca nuevo valor

    if (writeReg(AS5600_CONF + 1, value) != ESP_OK)
    {
        return false;
    }

    return true;
}

uint8_t AS5600::getPWMFrequency()
{
    uint8_t value = 0;
    if (readReg(AS5600_CONF + 1, value) != ESP_OK)
    {
        return 0xFF; // Valor reservado para indicar error
    }

    return (value >> 6) & 0x03;
}

bool AS5600::setSlowFilter(uint8_t mask)
{
    if (mask > 3)
        return false;

    uint8_t value = 0;
    if (readReg(AS5600_CONF, value) != ESP_OK)
    {
        return false;
    }

    value &= ~AS5600_CONF_SLOW_FILTER;         // limpiar bits 0 y 1
    value |= (mask & AS5600_CONF_SLOW_FILTER); // insertar nuevo valor

    if (writeReg(AS5600_CONF, value) != ESP_OK)
    {
        return false;
    }

    return true;
}

uint8_t AS5600::getSlowFilter()
{
    uint8_t value = 0;
    if (readReg(AS5600_CONF, value) != ESP_OK)
    {
        return 0xFF; // valor reservado que indica error
    }

    return value & 0x03;
}

bool AS5600::setFastFilter(uint8_t mask)
{
    if (mask > 7)
        return false; // porque son 3 bits (0–7)

    uint8_t value = 0;
    if (readReg(AS5600_CONF, value) != ESP_OK)
    {
        return false;
    }

    value &= ~AS5600_CONF_FAST_FILTER; // limpia bits 2–4
    value |= (mask << 2);              // coloca nuevo valor en posición

    if (writeReg(AS5600_CONF, value) != ESP_OK)
    {
        return false;
    }

    return true;
}

uint8_t AS5600::getFastFilter()
{
    uint8_t value = 0;
    if (readReg(AS5600_CONF, value) != ESP_OK)
    {
        return 0xFF; // valor reservado para error
    }

    return (value >> 2) & 0x07;
}

bool AS5600::setWatchDog(uint8_t mask)
{
    if (mask > 1)
        return false;

    uint8_t value = 0;
    if (readReg(AS5600_CONF, value) != ESP_OK)
    {
        return false;
    }

    value &= ~AS5600_CONF_WATCH_DOG; // limpia bit 5
    value |= (mask << 5);            // coloca nuevo valor en bit 5

    if (writeReg(AS5600_CONF, value) != ESP_OK)
    {
        return false;
    }

    return true;
}

uint8_t AS5600::getWatchDog()
{
    uint8_t value = 0;
    if (readReg(AS5600_CONF, value) != ESP_OK)
    {
        return 0xFF; // valor reservado para indicar error
    }

    return (value >> 5) & 0x01;
}

/////////////////////////////////////////////////////////
//
//  OUTPUT REGISTERS
//

uint16_t AS5600::rawAngle()
{
    uint16_t value = 0;

    if (readReg2(AS5600_RAW_ANGLE, value) != ESP_OK)
    {
        return 0xFFFF; // valor reservado que indica error
    }

    if (_offset > 0)
    {
        value += _offset;
    }

    value &= 0x0FFF; // El valor RAW es de 12 bits

    if ((_directionPin == AS5600_UNUSED_GPIO) &&
        (_direction == AS5600_COUNTERCLOCK_WISE))
    {
        value = (4096 - value) & 0x0FFF;
    }

    return value;
}

uint16_t AS5600::readAngle()
{
    uint16_t value = 0;

    if (readReg2(AS5600_ANGLE, value) != ESP_OK)
    {
        return _lastReadAngle; // Devuelve el último valor válido
    }

    if (_offset > 0)
    {
        value += _offset;
    }

    value &= 0x0FFF; // El ángulo solo usa 12 bits

    if ((_directionPin == AS5600_UNUSED_GPIO) &&
        (_direction == AS5600_COUNTERCLOCK_WISE))
    {
        value = (4096 - value) & 0x0FFF;
    }

    _lastReadAngle = value;
    return value;
}

bool AS5600::setOffset(float degrees)
{
    //  expect loss of precision.
    if (abs(degrees) > 36000)
        return false;
    bool neg = (degrees < 0);
    if (neg)
        degrees = -degrees;

    uint16_t offset = round(degrees * AS5600_DEGREES_TO_RAW);
    offset &= 0x0FFF;
    if (neg)
        offset = (4096 - offset) & 0x0FFF;
    _offset = offset;
    return true;
}

float AS5600::getOffset()
{
    return _offset * AS5600_RAW_TO_DEGREES;
}

bool AS5600::increaseOffset(float degrees)
{
    //  add offset to existing offset in degrees.
    return setOffset((_offset * AS5600_RAW_TO_DEGREES) + degrees);
}

/////////////////////////////////////////////////////////
//
//  STATUS REGISTERS
//
uint8_t AS5600::readStatus()
{
    uint8_t value = 0;
    if (readReg(AS5600_STATUS, value) != ESP_OK)
    {
        return 0xFF; // valor reservado para indicar error
    }

    return value;
}

uint8_t AS5600::readAGC()
{
    uint8_t value = 0;
    if (readReg(AS5600_AGC, value) != ESP_OK)
    {
        return 0xFF; // Valor reservado para indicar error
    }

    return value;
}

uint16_t AS5600::readMagnitude()
{
    uint16_t value = 0;
    if (readReg2(AS5600_MAGNITUDE, value) != ESP_OK)
    {
        return 0xFFFF; // valor reservado para indicar error
    }

    return value & 0x0FFF;
}

bool AS5600::detectMagnet()
{
    return (readStatus() & AS5600_MAGNET_DETECT) > 1;
}

bool AS5600::magnetTooStrong()
{
    return (readStatus() & AS5600_MAGNET_HIGH) > 1;
}

bool AS5600::magnetTooWeak()
{
    return (readStatus() & AS5600_MAGNET_LOW) > 1;
}

/////////////////////////////////////////////////////////
//
//  BURN COMMANDS
//
//  DO NOT UNCOMMENT - USE AT OWN RISK - READ DATASHEET
//
//  void AS5600::burnAngle()
//  {
//    writeReg(AS5600_BURN, x0x80);
//  }
//
//
//  See https://github.com/RobTillaart/AS5600/issues/38
//  void AS5600::burnSetting()
//  {
//    writeReg(AS5600_BURN, 0x40);
//    delay(5);
//    writeReg(AS5600_BURN, 0x01);
//    writeReg(AS5600_BURN, 0x11);
//    writeReg(AS5600_BURN, 0x10);
//    delay(5);
//  }

float AS5600::getAngularSpeed(uint8_t mode, bool update)
{
    if (update)
    {
        _lastReadAngle = readAngle();
        if (_error != AS5600_OK)
        {
            return NAN;
        }
    }
    //  default behaviour
    uint32_t now = esp_timer_get_time();
    int angle = _lastReadAngle;
    uint32_t deltaT = now - _lastMeasurement;
    int deltaA = angle - _lastAngle;

    //  assumption is that there is no more than 180° rotation
    //  between two consecutive measurements.
    //  => at least two measurements per rotation (preferred 4).
    if (deltaA > 2048)
        deltaA -= 4096;
    else if (deltaA < -2048)
        deltaA += 4096;
    float speed = (deltaA * 1e6) / deltaT;

    //  remember last time & angle
    _lastMeasurement = now;
    _lastAngle = angle;

    //  return radians, RPM or degrees.
    if (mode == AS5600_MODE_RADIANS)
    {
        return speed * AS5600_RAW_TO_RADIANS;
    }
    if (mode == AS5600_MODE_RPM)
    {
        return speed * AS5600_RAW_TO_RPM;
    }
    //  default return degrees
    return speed * AS5600_RAW_TO_DEGREES;
}

/////////////////////////////////////////////////////////
//
//  POSITION cumulative
//
int32_t AS5600::getCumulativePosition(bool update)
{
    if (update)
    {
        _lastReadAngle = readAngle();
        if (_error != AS5600_OK)
        {
            return _position; //  last known position.
        }
    }
    int16_t value = _lastReadAngle;

    //  whole rotation CW?
    //  less than half a circle
    if ((_lastPosition > 2048) && (value < (_lastPosition - 2048)))
    {
        _position = _position + 4096 - _lastPosition + value;
    }
    //  whole rotation CCW?
    //  less than half a circle
    else if ((value > 2048) && (_lastPosition < (value - 2048)))
    {
        _position = _position - 4096 - _lastPosition + value;
    }
    else
    {
        _position = _position - _lastPosition + value;
    }
    _lastPosition = value;

    return _position;
}

int32_t AS5600::getRevolutions()
{
    int32_t p = _position >> 12; //  divide by 4096
    if (p < 0)
        p++; //  correct negative values, See #65
    return p;
}

int32_t AS5600::resetPosition(int32_t position)
{
    int32_t old = _position;
    _position = position;
    return old;
}

int32_t AS5600::resetCumulativePosition(int32_t position)
{
    _lastPosition = readAngle();
    int32_t old = _position;
    _position = position;
    return old;
}

int AS5600::lastError()
{
    int value = _error;
    _error = AS5600_OK;
    return value;
}

/////////////////////////////////////////////////////////
//
//  PROTECTED AS5600
//
esp_err_t AS5600::readReg(uint8_t reg, uint8_t &data)
{
    _error = AS5600_OK;

    // Paso 1: Enviar la dirección del registro
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (_address << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, reg, true);
    i2c_master_stop(cmd);
    esp_err_t err = i2c_master_cmd_begin(_i2c_port, cmd, pdMS_TO_TICKS(100));
    i2c_cmd_link_delete(cmd);

    if (err != ESP_OK)
    {
        _error = AS5600_ERROR_I2C_READ_0;
        return err;
    }

    // Paso 2: Leer un byte del registro
    cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (_address << 1) | I2C_MASTER_READ, true);
    i2c_master_read_byte(cmd, &data, I2C_MASTER_NACK); // solo un byte
    i2c_master_stop(cmd);
    err = i2c_master_cmd_begin(_i2c_port, cmd, pdMS_TO_TICKS(100));
    i2c_cmd_link_delete(cmd);

    if (err != ESP_OK)
    {
        _error = AS5600_ERROR_I2C_READ_1;
        return err;
    }

    return ESP_OK;
}

esp_err_t AS5600::readReg2(uint8_t reg, uint16_t &data)
{
    _error = AS5600_OK;

    // Paso 1: Escribir el registro que se quiere leer
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (_address << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, reg, true);
    i2c_master_stop(cmd);
    esp_err_t err = i2c_master_cmd_begin(_i2c_port, cmd, pdMS_TO_TICKS(100));
    i2c_cmd_link_delete(cmd);

    if (err != ESP_OK)
    {
        _error = AS5600_ERROR_I2C_READ_2;
        return err;
    }

    // Paso 2: Leer 2 bytes
    uint8_t msb = 0, lsb = 0;
    cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (_address << 1) | I2C_MASTER_READ, true);
    i2c_master_read_byte(cmd, &msb, I2C_MASTER_ACK);  // primer byte
    i2c_master_read_byte(cmd, &lsb, I2C_MASTER_NACK); // segundo byte
    i2c_master_stop(cmd);
    err = i2c_master_cmd_begin(_i2c_port, cmd, pdMS_TO_TICKS(100));
    i2c_cmd_link_delete(cmd);

    if (err != ESP_OK)
    {
        _error = AS5600_ERROR_I2C_READ_3;
        return err;
    }

    data = (msb << 8) | lsb;
    return ESP_OK;
}

esp_err_t AS5600::writeReg(uint8_t reg, uint8_t value)
{
    _error = AS5600_OK;

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (_address << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, reg, true);
    i2c_master_write_byte(cmd, value, true);
    i2c_master_stop(cmd);

    esp_err_t err = i2c_master_cmd_begin(_i2c_port, cmd, pdMS_TO_TICKS(100));
    i2c_cmd_link_delete(cmd);

    if (err != ESP_OK)
    {
        _error = AS5600_ERROR_I2C_WRITE_0;
    }

    return err;
}

esp_err_t AS5600::writeReg2(uint8_t reg, uint16_t value)
{
    _error = AS5600_OK;

    uint8_t msb = (value >> 8) & 0xFF;
    uint8_t lsb = value & 0xFF;

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (_address << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, reg, true);
    i2c_master_write_byte(cmd, msb, true);
    i2c_master_write_byte(cmd, lsb, true);
    i2c_master_stop(cmd);

    esp_err_t err = i2c_master_cmd_begin(_i2c_port, cmd, pdMS_TO_TICKS(100));
    i2c_cmd_link_delete(cmd);

    if (err != ESP_OK)
    {
        _error = AS5600_ERROR_I2C_WRITE_1;
    }

    return err;
}

/////////////////////////////////////////////////////////////////////////////
//
//  AS5600L
//
AS5600L::AS5600L(i2c_port_t i2c_num, uint8_t address)
{
    _address = address;
    ; //  0x40 = default address AS5600L.
}

bool AS5600L::setAddress(uint8_t address)
{
    //  skip reserved I2C addresses
    if ((address < 8) || (address > 119))
        return false;

    //  note address need to be shifted 1 bit.
    writeReg(AS5600_I2CADDR, address << 1);
    writeReg(AS5600_I2CUPDT, address << 1);

    //  remember new address.
    _address = address;
    return true;
}

bool AS5600L::setI2CUPDT(uint8_t address)
{
    //  skip reserved I2C addresses
    if ((address < 8) || (address > 119))
        return false;
    writeReg(AS5600_I2CUPDT, address << 1);
    return true;
}

uint8_t AS5600L::getI2CUPDT()
{
    uint8_t value = 0;
    if (readReg(AS5600_I2CUPDT, value) != ESP_OK)
    {
        return 0xFF; // valor reservado para error
    }

    return (value >> 1) & 0x01; // solo bit 1
}

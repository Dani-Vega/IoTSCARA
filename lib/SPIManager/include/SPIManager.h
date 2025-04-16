#ifndef SPIMANAGER_H
#define SPIMANAGER_H

#include "driver/spi_master.h"
#include "driver/gpio.h"
#include <map>
#include <string>

class SPIManager {
private:
    spi_host_device_t host;
    std::map<std::string, spi_device_handle_t> devices;

public:
    SPIManager(spi_host_device_t spi_host);
    void initializeBus(gpio_num_t miso, gpio_num_t mosi, gpio_num_t sclk);
    void addDevice(const std::string& deviceName, gpio_num_t cs_pin, int clock_speed_hz);
    spi_device_handle_t getDevice(const std::string& deviceName);
};

#endif // SPIMANAGER_H

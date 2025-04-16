#ifndef ILI9341_H
#define ILI9341_H

#include <SPIManager.h>
#include <SDCard.h>
#include "driver/gpio.h"
#include "esp_log.h"
#include <string>
#include <vector>
#include <cstring>

class ILI9341 {
private:
    spi_device_handle_t spi;
    gpio_num_t pinDC;
    gpio_num_t pinRST;

    void sendCommand(uint8_t command);
    void sendData(const uint8_t* data, size_t length);

public:
    ILI9341();
    ~ILI9341();

    void setup(SPIManager& spiManager, const std::string& deviceName, gpio_num_t dc, gpio_num_t rst);
    void clearScreen(uint16_t color);
    void setAddressWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1);
    void drawImage(uint16_t x, uint16_t y, uint16_t width, uint16_t height, const uint16_t* imageData);
    void displayJPEG(const std::string& jpegPath, uint16_t width, uint16_t height);
    void slideshowJPEG(SDCard& sdCard, const std::string& directory, uint16_t width, uint16_t height, uint16_t duration_ms);
};

#endif // ILI9341_H

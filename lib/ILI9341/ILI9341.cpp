#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "ILI9341.h"
#include <vector>
#include <cstdio>
#include <esp_timer.h>

static const char* TAG_ILI9341 = "ILI9341";

ILI9341::ILI9341() : spi(nullptr), pinDC(GPIO_NUM_NC), pinRST(GPIO_NUM_NC) {}

ILI9341::~ILI9341() {}

void ILI9341::setup(SPIManager& spiManager, const std::string& deviceName, gpio_num_t dc, gpio_num_t rst) {
    spi = spiManager.getDevice(deviceName);
    if (!spi) {
        ESP_LOGE(TAG_ILI9341, "Failed to initialize ILI9341: SPI device not found");
        return;
    }

    pinDC = dc;
    pinRST = rst;

    gpio_set_direction(pinDC, GPIO_MODE_OUTPUT);
    gpio_set_direction(pinRST, GPIO_MODE_OUTPUT);

    gpio_set_level(pinRST, 0);
    vTaskDelay(pdMS_TO_TICKS(100));
    gpio_set_level(pinRST, 1);
    vTaskDelay(pdMS_TO_TICKS(100));

    ESP_LOGI(TAG_ILI9341, "ILI9341 setup completed.");
}

void ILI9341::sendCommand(uint8_t command) {
    gpio_set_level(pinDC, 0); // DC = 0 para comando
    spi_transaction_t t = {};
    t.length = 8;
    t.tx_buffer = &command;
    ESP_ERROR_CHECK(spi_device_transmit(spi, &t));
}

void ILI9341::sendData(const uint8_t* data, size_t length) {
    gpio_set_level(pinDC, 1); // DC = 1 para datos
    spi_transaction_t t = {};
    t.length = length * 8;
    t.tx_buffer = data;
    ESP_ERROR_CHECK(spi_device_transmit(spi, &t));
}

void ILI9341::clearScreen(uint16_t color) {
    uint8_t color_high = (color >> 8) & 0xFF;
    uint8_t color_low = color & 0xFF;
    setAddressWindow(0, 0, 239, 319);
    sendCommand(0x2C);

    for (int i = 0; i < 240 * 320; ++i) {
        uint8_t data[2] = {color_high, color_low};
        sendData(data, 2);
    }

    ESP_LOGI(TAG_ILI9341, "Screen cleared with color: 0x%04X", color);
}

void ILI9341::setAddressWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
    uint8_t data[4];
    sendCommand(0x2A);
    data[0] = (x0 >> 8) & 0xFF; data[1] = x0 & 0xFF;
    data[2] = (x1 >> 8) & 0xFF; data[3] = x1 & 0xFF;
    sendData(data, 4);

    sendCommand(0x2B);
    data[0] = (y0 >> 8) & 0xFF; data[1] = y0 & 0xFF;
    data[2] = (y1 >> 8) & 0xFF; data[3] = y1 & 0xFF;
    sendData(data, 4);

    sendCommand(0x2C);
}

void ILI9341::drawImage(uint16_t x, uint16_t y, uint16_t width, uint16_t height, const uint16_t* imageData) {
    setAddressWindow(x, y, x + width - 1, y + height - 1);

    for (size_t i = 0; i < width * height; ++i) {
        uint8_t data[2] = {
            static_cast<uint8_t>(imageData[i] >> 8),
            static_cast<uint8_t>(imageData[i] & 0xFF)
        };
        sendData(data, 2);
    }

    ESP_LOGI(TAG_ILI9341, "Image drawn at (%d, %d) with size %dx%d", x, y, width, height);
}

void ILI9341::displayJPEG(const std::string& jpegPath, uint16_t width, uint16_t height) {
    int imgWidth, imgHeight, channels;
    unsigned char* imgData = stbi_load(jpegPath.c_str(), &imgWidth, &imgHeight, &channels, 3);
    if (!imgData) {
        ESP_LOGE(TAG_ILI9341, "Failed to load JPEG: %s", jpegPath.c_str());
        return;
    }

    uint16_t pixelBuffer[imgWidth * imgHeight];
    for (int y = 0; y < imgHeight; ++y) {
        for (int x = 0; x < imgWidth; ++x) {
            int index = (y * imgWidth + x) * 3;
            uint16_t color = ((imgData[index] & 0xF8) << 8) | ((imgData[index + 1] & 0xFC) << 3) | (imgData[index + 2] >> 3);
            pixelBuffer[y * imgWidth + x] = color;
        }
    }

    drawImage(0, 0, imgWidth, imgHeight, pixelBuffer);
    stbi_image_free(imgData);

    ESP_LOGI(TAG_ILI9341, "JPEG displayed from: %s", jpegPath.c_str());
}

void ILI9341::slideshowJPEG(SDCard& sdCard, const std::string& directory, uint16_t width, uint16_t height, uint16_t duration_ms) {
    static size_t currentImage = 0;
    static uint32_t lastUpdateTime = 0;

    std::vector<std::string> imagePaths = sdCard.getFilesWithExtension(directory, ".jpg");

    if (imagePaths.empty()) {
        ESP_LOGE(TAG_ILI9341, "No JPEG files found in directory: %s", directory.c_str());
        return;
    }

    uint32_t now = esp_timer_get_time() / 1000; // Tiempo actual en ms

    if (now - lastUpdateTime >= duration_ms) {
        displayJPEG(imagePaths[currentImage], width, height);
        currentImage = (currentImage + 1) % imagePaths.size();
        lastUpdateTime = now;
    }
}

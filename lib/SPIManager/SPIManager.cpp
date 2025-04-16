// SPIManager.cpp
#include "SPIManager.h"
#include "esp_log.h"

static const char* TAG = "SPIManager";

SPIManager::SPIManager(spi_host_device_t spi_host) : host(spi_host) {}

void SPIManager::initializeBus(gpio_num_t miso, gpio_num_t mosi, gpio_num_t sclk) {
    spi_bus_config_t buscfg = {
        .mosi_io_num = mosi,
        .miso_io_num = miso,
        .sclk_io_num = sclk,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4096,
    };
    ESP_ERROR_CHECK(spi_bus_initialize(host, &buscfg, SPI_DMA_CH_AUTO));
    ESP_LOGI(TAG, "SPI bus initialized.");
}

void SPIManager::addDevice(const std::string& deviceName, gpio_num_t cs_pin, int clock_speed_hz) {
    spi_device_interface_config_t devcfg = {
        .mode = 0, // SPI mode 0
        .clock_speed_hz = clock_speed_hz,
        .spics_io_num = cs_pin,
        .queue_size = 7,
        .pre_cb = nullptr,
    };

    spi_device_handle_t handle;
    ESP_ERROR_CHECK(spi_bus_add_device(host, &devcfg, &handle));
    devices[deviceName] = handle;
    ESP_LOGI(TAG, "Device '%s' added with CS pin %d.", deviceName.c_str(), cs_pin);
}

spi_device_handle_t SPIManager::getDevice(const std::string& deviceName) {
    if (devices.find(deviceName) != devices.end()) {
        return devices[deviceName];
    }
    ESP_LOGE(TAG, "Device '%s' not found!", deviceName.c_str());
    return nullptr;
}

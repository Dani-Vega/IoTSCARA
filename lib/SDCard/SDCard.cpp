#include "SDCard.h"
#include "esp_log.h"
#include <dirent.h>
#include <algorithm>
#include <cstdio>

static const char* TAG_SD = "SDCard";

SDCard::SDCard() : card(nullptr) {}

SDCard::~SDCard() {
    if (card) {
        esp_vfs_fat_sdcard_unmount(mountPoint.c_str(), card);
        ESP_LOGI(TAG_SD, "SD card unmounted from %s", mountPoint.c_str());
    }
}

bool SDCard::setup(SPIManager& spiManager, const std::string& deviceName, gpio_num_t csPin, const char* mount_point, bool format_if_mount_failed, int max_files) {
    mountPoint = mount_point;

    // Agregar tarjeta SD como dispositivo SPI
    spiManager.addDevice(deviceName, csPin, 10000000); // Frecuencia 10 MHz
    spi_device_handle_t spiHandle = spiManager.getDevice(deviceName);
    if (!spiHandle) {
        ESP_LOGE(TAG_SD, "Failed to add SD card to SPIManager");
        return false;
    }

    sdmmc_host_t host = SDSPI_HOST_DEFAULT();

    sdspi_device_config_t slot_config = {};
    slot_config.gpio_cs = csPin;
    slot_config.host_id = SPI2_HOST;

    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = format_if_mount_failed,
        .max_files = max_files,
        .allocation_unit_size = 16 * 1024
    };

    esp_err_t ret = esp_vfs_fat_sdspi_mount(mount_point, &host, &slot_config, &mount_config, &card);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG_SD, "Failed to mount SD card: %s", esp_err_to_name(ret));
        return false;
    }

    ESP_LOGI(TAG_SD, "SD card mounted at %s", mount_point);
    return true;
}

std::vector<std::string> SDCard::listFilesInDirectory(const std::string& directory) {
    std::vector<std::string> fileList;
    DIR* dir = opendir(directory.c_str());
    if (!dir) {
        ESP_LOGE(TAG_SD, "Failed to open directory: %s", directory.c_str());
        return fileList;
    }

    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        if (entry->d_type == DT_REG) { // Archivos regulares
            fileList.push_back(directory + "/" + entry->d_name);
        }
    }

    closedir(dir);

    std::sort(fileList.begin(), fileList.end());
    return fileList;
}

std::vector<std::string> SDCard::getFilesWithExtension(const std::string& directory, const std::string& extension) {
    std::vector<std::string> allFiles = listFilesInDirectory(directory);
    std::vector<std::string> filteredFiles;

    for (const auto& file : allFiles) {
        if (file.find(extension) != std::string::npos) {
            filteredFiles.push_back(file);
        }
    }

    return filteredFiles;
}

bool SDCard::readFile(const std::string& path, std::string& content) {
    FILE* f = fopen(path.c_str(), "r");
    if (!f) {
        ESP_LOGE(TAG_SD, "Failed to open file %s for reading", path.c_str());
        return false;
    }

    fseek(f, 0, SEEK_END);
    size_t size = ftell(f);
    fseek(f, 0, SEEK_SET);

    content.resize(size);
    fread(&content[0], 1, size, f);
    fclose(f);

    ESP_LOGI(TAG_SD, "File %s read successfully", path.c_str());
    return true;
}

std::string SDCard::getMountPoint() const {
    return mountPoint;
}

int SDCard::getNextFileNumber(const std::string& directory) {
    DIR* dir = opendir(directory.c_str());
    if (!dir) {
        ESP_LOGE(TAG_SD, "Failed to open directory: %s", directory.c_str());
        return 1; // Si el directorio no existe, empezar con 1
    }

    struct dirent* entry;
    int maxNumber = 0;

    while ((entry = readdir(dir)) != nullptr) {
        if (entry->d_type == DT_REG) { // Archivos regulares
            std::string filename = entry->d_name;
            size_t dotPos = filename.find_last_of('.');
            if (dotPos != std::string::npos) {
                std::string baseName = filename.substr(0, dotPos);
                try {
                    int number = std::stoi(baseName);
                    if (number > maxNumber) {
                        maxNumber = number;
                    }
                } catch (...) {
                    // Ignorar archivos que no sean números
                }
            }
        }
    }

    closedir(dir);
    return maxNumber + 1; // Devuelve el siguiente número
}

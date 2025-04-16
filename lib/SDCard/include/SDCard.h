#ifndef SDCARD_H
#define SDCARD_H

#include "SPIManager.h"
#include "driver/sdspi_host.h"
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include <vector>
#include <string>

class SDCard {
private:
    std::string mountPoint;
    sdmmc_card_t* card;

public:
    SDCard();
    ~SDCard();

    bool setup(SPIManager& spiManager, const std::string& deviceName, gpio_num_t csPin, const char* mount_point = "/sdcard", bool format_if_mount_failed = false, int max_files = 5);
    std::vector<std::string> listFilesInDirectory(const std::string& directory);
    std::vector<std::string> getFilesWithExtension(const std::string& directory, const std::string& extension);
    bool readFile(const std::string& path, std::string& content);
    std::string getMountPoint() const;
    int getNextFileNumber(const std::string& directory);
};

#endif // SDCARD_H

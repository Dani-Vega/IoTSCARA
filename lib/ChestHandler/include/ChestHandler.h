#ifndef CHESTHANDLER_H
#define CHESTHANDLER_H

#include "nvs_flash.h"
#include "nvs.h"
#include "esp_log.h"

class ChestHandler
{
public:
    ChestHandler();                       // Constructor
    ~ChestHandler();                      // Destructor
    bool getChestState() const;           // Devuelve el valor de _openChest
    void saveChestState(bool newState);   // Guarda el valor en la memoria flash

private:
    bool _openChest;                      // Variable booleana privada
    void loadChestState();                // Carga el valor desde NVS
    nvs_handle_t _nvsHandle;              // Maneja el NVS
    static constexpr const char* TAG = "ChestHandler";
    static constexpr const char* NAMESPACE = "chest_storage";
    static constexpr const char* KEY = "chest_open";
};

#endif // CHESTHANDLER_H

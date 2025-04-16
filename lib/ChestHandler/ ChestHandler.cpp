#include "ChestHandler.h"

ChestHandler::ChestHandler() : _openChest(false), _nvsHandle(0)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_ERROR_CHECK(nvs_flash_init());
    }
    loadChestState();
}

ChestHandler::~ChestHandler()
{
    if (_nvsHandle != 0)
    {
        nvs_close(_nvsHandle);
    }
}

void ChestHandler::loadChestState()
{
    esp_err_t err = nvs_open(NAMESPACE, NVS_READONLY, &_nvsHandle);
    if (err == ESP_OK)
    {
        uint8_t chestState = 0;
        if (nvs_get_u8(_nvsHandle, KEY, &chestState) == ESP_OK)
        {
            _openChest = chestState;
            ESP_LOGI(TAG, "Chest state loaded: %s", _openChest ? "OPEN" : "CLOSED");
        }
        else
        {
            ESP_LOGI(TAG, "No previous chest state found, defaulting to CLOSED.");
        }
    }
    else
    {
        ESP_LOGE(TAG, "Failed to open NVS namespace.");
    }
}

void ChestHandler::saveChestState(bool newState)
{
    if (newState == _openChest)
    {
        ESP_LOGI(TAG, "New state is identical to the current state. No need to save.");
        return; // Evita redundancias
    }

    esp_err_t err = nvs_open(NAMESPACE, NVS_READWRITE, &_nvsHandle);
    if (err == ESP_OK)
    {
        _openChest = newState;
        ESP_ERROR_CHECK(nvs_set_u8(_nvsHandle, KEY, static_cast<uint8_t>(_openChest)));
        ESP_ERROR_CHECK(nvs_commit(_nvsHandle));
        ESP_LOGI(TAG, "Chest state saved: %s", _openChest ? "OPEN" : "CLOSED");
    }
    else
    {
        ESP_LOGE(TAG, "Failed to open NVS namespace for writing.");
    }
}

bool ChestHandler::getChestState() const
{
    return _openChest;
}

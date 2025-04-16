#include "OTAManager.h"
#include "esp_ota_ops.h"

const char *OTAManager::TAG = "OTA_UPDATE";

OTAManager::OTAManager() {}
OTAManager::~OTAManager() {}

void OTAManager::perform_ota_update(const char *url)
{
    esp_http_client_config_t http_config = {
        .url = url,
        .cert_pem = CERT_USERTRUST_RSA_ROOT,
        .buffer_size = 4096,
    };

    esp_https_ota_config_t ota_config = {
        .http_config = &http_config,
    };

    ESP_LOGI(TAG, "Starting OTA...");
    esp_err_t ret = esp_https_ota(&ota_config);

    if (ret == ESP_OK)
    {
        ESP_LOGI(TAG, "OTA update successful. Marking as pending and rebooting...");
        esp_ota_mark_app_valid_cancel_rollback();
        esp_restart();
    }
    else
    {
        ESP_LOGE(TAG, "OTA update failed: %s", esp_err_to_name(ret));
    }
}

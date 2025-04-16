#include "WiFiManager.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "nvs.h"
#include <string.h>

static const char *TAG = "WiFiManager";

WiFiManager::WiFiManager(const std::string &ssid, const std::string &password, bool mode)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_ERROR_CHECK(nvs_flash_init());
    }
    if (mode)
    {
        _ssid = ssid;
        _password = password;
    }
    else
    {
        _originalSsid = ssid;
        _originalPassword = password;
        loadCredentialsFromNVS();
    }
}

WiFiManager::~WiFiManager()
{
    stopWiFi();
}

bool WiFiManager::start()
{
    stopWiFi();
    configureWiFi();

    if (!_ssid.empty() && !_password.empty())
    {
        if (tryConnectToSTA())
        {
            ESP_LOGI(TAG, "Started in STA mode.");
            return true;
        }
        else
        {
            ESP_LOGW(TAG, "Failed to connect in STA mode.");
        }
    }
    else
    {
        ESP_LOGW(TAG, "No WiFi credentials found. Unable to start in STA mode.");
    }
    return false;
}

void WiFiManager::initAP()
{
    ESP_LOGI(TAG, "Initializing WiFi in Access Point mode...");

    esp_netif_t *netif = esp_netif_create_default_wifi_ap();
    if (!netif)
    {
        ESP_LOGE(TAG, "Failed to create default Wi-Fi AP interface");
        return;
    }

    ESP_ERROR_CHECK(esp_netif_dhcps_start(netif));

    wifi_config_t wifiConfig = {};
    strncpy((char *)wifiConfig.ap.ssid, _originalSsid.c_str(), sizeof(wifiConfig.ap.ssid) - 1);
    strncpy((char *)wifiConfig.ap.password, _originalPassword.c_str(), sizeof(wifiConfig.ap.password) - 1);
    wifiConfig.ap.ssid_len = _originalSsid.length();
    wifiConfig.ap.max_connection = 4;
    wifiConfig.ap.authmode = _originalPassword.empty() ? WIFI_AUTH_OPEN : WIFI_AUTH_WPA2_PSK;

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifiConfig));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "Access Point initialized with SSID: %s", _originalSsid.c_str());
}

void WiFiManager::wifiEventHandler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    WiFiManager *instance = static_cast<WiFiManager *>(arg);
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_STACONNECTED)
    {
        auto *event = (wifi_event_ap_staconnected_t *)event_data;
        ESP_LOGI(TAG, "Client connected, MAC: %02x:%02x:%02x:%02x:%02x:%02x, AID: %d",
                 event->mac[0], event->mac[1], event->mac[2],
                 event->mac[3], event->mac[4], event->mac[5], event->aid);
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_STADISCONNECTED)
    {
        auto *event = (wifi_event_ap_stadisconnected_t *)event_data;
        ESP_LOGI(TAG, "Client disconnected, MAC: %02x:%02x:%02x:%02x:%02x:%02x, AID: %d",
                 event->mac[0], event->mac[1], event->mac[2],
                 event->mac[3], event->mac[4], event->mac[5], event->aid);
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_AP_STAIPASSIGNED)
    {
        auto *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG, "IP assigned to client: " IPSTR, IP2STR(&event->ip_info.ip));
        ESP_LOGI(TAG, "Netmask: " IPSTR, IP2STR(&event->ip_info.netmask));
        ESP_LOGI(TAG, "Gateway: " IPSTR, IP2STR(&event->ip_info.gw));
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        ESP_LOGI(TAG, "IP assigned.");
        instance->connectionSuccessful = true; // Marcar conexión como exitosa
    }
}

void WiFiManager::switchToSTA(const std::string &newSsid, const std::string &newPassword)
{
    ESP_LOGI(TAG, "Switching to STA mode...");

    saveCredentialsToNVS(newSsid, newPassword);

    this->_ssid = newSsid;
    this->_password = newPassword;

    if (!tryConnectToSTA())
    {
        ESP_LOGW(TAG, "Connection failed. Reverting to Access Point mode.");
        resetNVS();
        initAP();
    }
}

void WiFiManager::stopWiFi()
{
    ESP_LOGI(TAG, "Stopping WiFi...");
    esp_err_t err = esp_wifi_stop();
    if (err == ESP_ERR_WIFI_NOT_INIT)
    {
        ESP_LOGW(TAG, "WiFi not initialized, skipping stop.");
    }
    else
    {
        ESP_ERROR_CHECK(err);
    }
    esp_wifi_deinit();

    if (_sta_netif)
    {
        esp_netif_destroy(_sta_netif);
        _sta_netif = nullptr;
    }
}

void WiFiManager::configureWiFi()
{
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    // Registra el manejador para eventos IP y WiFi
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, wifiEventHandler, this, nullptr));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, wifiEventHandler, this, nullptr));
}

void WiFiManager::loadCredentialsFromNVS()
{
    nvs_handle_t nvsHandle;
    esp_err_t err = nvs_open("storage", NVS_READONLY, &nvsHandle);
    if (err == ESP_OK)
    {
        size_t ssidSize = 32;
        size_t passSize = 64;
        char ssidBuffer[32];
        char passBuffer[64];

        nvs_get_str(nvsHandle, "wifi_ssid", ssidBuffer, &ssidSize);
        nvs_get_str(nvsHandle, "wifi_pass", passBuffer, &passSize);

        _ssid = std::string(ssidBuffer);
        _password = std::string(passBuffer);
        nvs_close(nvsHandle);
        ESP_LOGI(TAG, "Loaded WiFi credentials from NVS.");
    }
    else
    {
        ESP_LOGW(TAG, "No WiFi credentials stored in NVS.");
    }
}

void WiFiManager::saveCredentialsToNVS(const std::string &newSsid, const std::string &newPassword)
{
    nvs_handle_t nvsHandle;
    ESP_ERROR_CHECK(nvs_open("storage", NVS_READWRITE, &nvsHandle));
    ESP_ERROR_CHECK(nvs_set_str(nvsHandle, "wifi_ssid", newSsid.c_str()));
    ESP_ERROR_CHECK(nvs_set_str(nvsHandle, "wifi_pass", newPassword.c_str()));
    ESP_ERROR_CHECK(nvs_commit(nvsHandle));
    nvs_close(nvsHandle);
    ESP_LOGI(TAG, "Saved WiFi credentials to NVS.");
}

bool WiFiManager::tryConnectToSTA()
{
    ESP_LOGI(TAG, "Attempting to connect to WiFi (STA mode)...");

    stopWiFi();  // Esto ya destruye _sta_netif si existía

    _sta_netif = esp_netif_create_default_wifi_sta();
    if (!_sta_netif)
    {
        ESP_LOGE(TAG, "Failed to create default Wi-Fi STA interface");
        return false;
    }

    configureWiFi();

    wifi_config_t wifiConfig = {};
    strncpy((char *)wifiConfig.sta.ssid, _ssid.c_str(), sizeof(wifiConfig.sta.ssid) - 1);
    strncpy((char *)wifiConfig.sta.password, _password.c_str(), sizeof(wifiConfig.sta.password) - 1);

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifiConfig));
    ESP_ERROR_CHECK(esp_wifi_start());

    for (int retryCount = 0; retryCount < maxRetries; retryCount++)
    {
        ESP_LOGI(TAG, "Connecting... Attempt %d/%d", retryCount + 1, maxRetries);
        esp_err_t err = esp_wifi_connect();
        if (err != ESP_OK)
        {
            ESP_LOGW(TAG, "WiFi connect failed: %s", esp_err_to_name(err));
        }

        vTaskDelay(pdMS_TO_TICKS(2000));

        if (connectionSuccessful)
        {
            ESP_LOGI(TAG, "Connected to WiFi successfully.");
            return true;
        }
    }

    ESP_LOGE(TAG, "Failed to connect to WiFi after %d attempts.", maxRetries);
    return false;
}

void WiFiManager::resetNVS()
{
    ESP_LOGI(TAG, "Resetting WiFi credentials in NVS...");
    nvs_handle_t nvsHandle;
    ESP_ERROR_CHECK(nvs_open("storage", NVS_READWRITE, &nvsHandle));
    ESP_ERROR_CHECK(nvs_erase_key(nvsHandle, "wifi_ssid"));
    ESP_ERROR_CHECK(nvs_erase_key(nvsHandle, "wifi_pass"));
    ESP_ERROR_CHECK(nvs_commit(nvsHandle));
    nvs_close(nvsHandle);
}

bool WiFiManager::isConnectedToInternet()
{
    if (connectionSuccessful)
    {
        return true;
    }
    return false;
}

std::vector<std::string> WiFiManager::scanNetworks()
{
    ESP_LOGI(TAG, "Starting WiFi scan...");
    wifi_scan_config_t scanConfig = {};
    scanConfig.show_hidden = true;

    if (esp_wifi_scan_start(&scanConfig, true) == ESP_OK)
    {
        uint16_t numNetworks = 0;
        ESP_ERROR_CHECK(esp_wifi_scan_get_ap_num(&numNetworks));

        wifi_ap_record_t *apRecords = (wifi_ap_record_t *)malloc(numNetworks * sizeof(wifi_ap_record_t));
        std::vector<std::string> networks;

        if (apRecords && esp_wifi_scan_get_ap_records(&numNetworks, apRecords) == ESP_OK)
        {
            for (int i = 0; i < numNetworks; ++i)
            {
                // Convertir SSID a cadena de texto
                std::string ssid = reinterpret_cast<const char *>(apRecords[i].ssid);

                // Verificar que el SSID no esté vacío y que no sea duplicado
                if (!ssid.empty() && std::find(networks.begin(), networks.end(), ssid) == networks.end())
                {
                    networks.push_back(ssid);
                }
            }
        }
        free(apRecords);

        ESP_LOGI(TAG, "Found %d unique WiFi networks.", networks.size());
        return networks;
    }

    ESP_LOGW(TAG, "WiFi scan failed.");
    return {};
}

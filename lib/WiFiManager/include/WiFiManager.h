#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include "esp_wifi.h"
#include "esp_event.h"
#include <string>
#include <string>
#include "esp_err.h"
#include <string>
#include <algorithm>
#include <vector>

class WiFiManager
{
public:
    WiFiManager(const std::string &ssid = "", const std::string &password = "", bool mode = 0); // Mode 0 = AP, Mode 1 = STA (Credentials)  
    ~WiFiManager();

    bool start();
    void initAP();
    void switchToSTA(const std::string &newSsid, const std::string &newPassword);
    void stopWiFi();
    bool isConnectedToInternet();
    std::vector<std::string> scanNetworks();
    void saveCredentialsToNVS(const std::string &newSsid, const std::string &newPassword);

private:
    void configureWiFi();
    void loadCredentialsFromNVS();
    bool tryConnectToSTA();
    void resetNVS();

    static void wifiEventHandler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);

    esp_netif_t* _sta_netif = nullptr;
    std::string _ssid;
    std::string _password;
    std::string _originalSsid;
    std::string _originalPassword;
    static constexpr int maxRetries = 5;
    bool connectionSuccessful = false;
};

#endif // WIFIMANAGER_H

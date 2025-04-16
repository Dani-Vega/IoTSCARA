// PS4Controller.h
#include "esp_err.h"
#include "esp_event.h"
#include "esp_gap_bt_api.h"
#include "esp_hidh.h"
#include "esp_bt_defs.h"

class PS4Controller {
public:
    PS4Controller();
    ~PS4Controller();

    esp_err_t begin();
    void end();

    bool isConnected() const;
    uint8_t getBatteryLevel() const;

private:
    static bool connected;
    static uint8_t batteryLevel;

    static void gapCallback(esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t *param);
    static void hidEventHandler(void *arg, esp_event_base_t base, int32_t id, void *event_data);
    static void parseInput(const uint8_t *data, size_t length);

    esp_err_t initNVS();
    esp_err_t initBluetooth();
    esp_err_t registerHIDHost();
};

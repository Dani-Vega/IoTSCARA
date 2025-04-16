#ifndef __SIMPLE_BLE_H__
#define __SIMPLE_BLE_H__

#include "esp_log.h"
#include "nvs_flash.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "host/ble_hs.h"
#include "host/util/util.h"
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"
#include "nimble/ble.h"

// UUIDs del HM-10 (usado por Dabble)
static const ble_uuid16_t svc_uuid = BLE_UUID16_INIT(0xFFE0);  // Service UUID
static const ble_uuid16_t char_uuid = BLE_UUID16_INIT(0xFFE1);  // Characteristic UUID

class SimpleBLE {
public:
    void begin(const char* deviceName);
    bool available();
    int read(char buffer[], uint32_t length);
    static void write(char data[], size_t length);

private:
    static int bleGapEvent(struct ble_gap_event* event, void* arg);
    static void bleAdvertise();
    static void bleOnSync();
    static void bleOnReset(int reason);
    static void bleHostTask(void* param);
    static int gattHandler(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt* ctxt, void* arg);

    static uint8_t own_addr_type;
    static uint16_t gatt_char_val_handle;
    static QueueHandle_t uart_queue;
    static uint16_t conn_handle;

    static void printConnDesc(struct ble_gap_conn_desc* desc);
    static void ble_store_config_init(void);
    static void gatt_svr_register_cb(struct ble_gatt_register_ctxt *ctxt, void *arg);
    static const struct ble_gatt_svc_def gatt_svr_defs[];

    static bool _data_available;
    static char _message_in[128];
    static char _message_out[128];
    static uint16_t _length_in;
    static size_t _length_out;
};

#endif // __SIMPLE_BLE_H__

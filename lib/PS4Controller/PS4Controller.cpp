// PS4Controller.cpp
#include "PS4Controller.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_bt_device.h"
#include "esp_gap_bt_api.h"
#include "esp_hidh.h"
#include <string.h>

static const char *TAG = "PS4Controller";
bool PS4Controller::connected = false;
uint8_t PS4Controller::batteryLevel = 0;

static const char *TARGET_NAME = "Wireless Controller";
static esp_bd_addr_t last_found_addr = {0};

// Estructura de entrada para el control PS4 (versión simplificada de report 0x11)
typedef struct __attribute__((packed)) {
    uint8_t x, y;
    uint8_t rx, ry;
    uint8_t buttons[3];
    uint8_t brake;
    uint8_t throttle;
    uint16_t sensor_timestamp;
    uint8_t sensor_temperature;
    uint16_t gyro[3];   // x, y, z
    uint16_t accel[3];  // x, y, z
    uint8_t reserved[5];
    uint8_t status[2];  // incluye nivel de batería en los 4 bits menos significativos de status[0]
} ds4_input_report_11_t;

PS4Controller::PS4Controller() {}

PS4Controller::~PS4Controller() {
    end();
}

esp_err_t PS4Controller::initNVS() {
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    return ret;
}

esp_err_t PS4Controller::initBluetooth() {
    ESP_LOGI(TAG, "Liberando memoria BLE...");
    esp_bt_controller_mem_release(ESP_BT_MODE_BLE);

    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_bt_controller_init(&bt_cfg));
    ESP_ERROR_CHECK(esp_bt_controller_enable(ESP_BT_MODE_CLASSIC_BT));

    ESP_ERROR_CHECK(esp_bluedroid_init());
    ESP_ERROR_CHECK(esp_bluedroid_enable());

    ESP_ERROR_CHECK(esp_bt_dev_set_device_name("ESP32-PS4"));
    ESP_ERROR_CHECK(esp_bt_gap_register_callback(PS4Controller::gapCallback));
    ESP_ERROR_CHECK(esp_bt_gap_set_scan_mode(ESP_BT_CONNECTABLE, ESP_BT_GENERAL_DISCOVERABLE));

    ESP_ERROR_CHECK(registerHIDHost()); // Register HID Host before scanning

    ESP_LOGI(TAG, "Iniciando escaneo de dispositivos BT...");
    ESP_ERROR_CHECK(esp_bt_gap_start_discovery(ESP_BT_INQ_MODE_GENERAL_INQUIRY, 10, 0));

    return ESP_OK;
}

esp_err_t PS4Controller::registerHIDHost() {
    esp_hidh_config_t config = {
        .callback = PS4Controller::hidEventHandler,
        .event_stack_size = 4096,
        .callback_arg = nullptr
    };
    return esp_hidh_init(&config);
}

esp_err_t PS4Controller::begin() {
    ESP_ERROR_CHECK(initNVS());
    ESP_ERROR_CHECK(initBluetooth());
    return ESP_OK;
}

void PS4Controller::end() {
    esp_hidh_deinit();
    esp_bluedroid_disable();
    esp_bluedroid_deinit();
    esp_bt_controller_disable();
    esp_bt_controller_deinit();
}

bool PS4Controller::isConnected() const {
    return connected;
}

uint8_t PS4Controller::getBatteryLevel() const {
    return batteryLevel;
}

void PS4Controller::parseInput(const uint8_t *data, size_t length) {
    if (length < 2) {
        ESP_LOGW(TAG, "Reporte demasiado corto (%d bytes)", length);
        return;
    }

    uint8_t report_id = data[0];

    if (report_id == 0x01 && length == 10) {
        const ds4_input_report_11_t *report = (const ds4_input_report_11_t *)&data[1];
        int lx = (int)report->x - 127;
        int ly = (int)report->y - 127;
        int rx = (int)report->rx - 127;
        int ry = (int)report->ry - 127;

        ESP_LOGI(TAG, "Joystick Izq: (%d, %d), Der: (%d, %d)", lx, ly, rx, ry);
        ESP_LOGI(TAG, "Freno: %d, Acelerador: %d", report->brake, report->throttle);
        return;

    } else if (report_id == 0x11 && length >= 78) {
        const ds4_input_report_11_t *report = (const ds4_input_report_11_t *)&data[3];

        int lx = (int)report->x - 127;
        int ly = (int)report->y - 127;
        int rx = (int)report->rx - 127;
        int ry = (int)report->ry - 127;

        ESP_LOGI(TAG, "Joystick Izq: (%d, %d), Der: (%d, %d)", lx, ly, rx, ry);

        uint8_t b0 = report->buttons[0];
        uint8_t b1 = report->buttons[1];
        uint8_t b2 = report->buttons[2];

        if (b0 & 0x10) ESP_LOGI(TAG, "Botón X presionado");
        if (b0 & 0x20) ESP_LOGI(TAG, "Botón A presionado");
        if (b0 & 0x40) ESP_LOGI(TAG, "Botón B presionado");
        if (b0 & 0x80) ESP_LOGI(TAG, "Botón Y presionado");
        if (b1 & 0x01) ESP_LOGI(TAG, "Botón L1");
        if (b1 & 0x02) ESP_LOGI(TAG, "Botón R1");
        if (b1 & 0x10) ESP_LOGI(TAG, "Share");
        if (b1 & 0x20) ESP_LOGI(TAG, "Options");
        if (b2 & 0x01) ESP_LOGI(TAG, "Botón PS");

        ESP_LOGI(TAG, "Freno: %d, Acelerador: %d", report->brake, report->throttle);

        ESP_LOGI(TAG, "Giroscopio: X=%d Y=%d Z=%d",
                 (int16_t)report->gyro[0], (int16_t)report->gyro[1], (int16_t)report->gyro[2]);

        ESP_LOGI(TAG, "Acelerómetro: X=%d Y=%d Z=%d",
                 (int16_t)report->accel[0], (int16_t)report->accel[1], (int16_t)report->accel[2]);

        uint8_t raw_batt = report->status[0] & 0x0F;
        batteryLevel = raw_batt * 10;
        ESP_LOGI(TAG, "Batería reportada (raw): %d -> %d%%", raw_batt, batteryLevel);

        return;

    } else {
        ESP_LOGW(TAG, "Tipo de reporte no esperado: 0x%02x (%d bytes)", report_id, length);
    }
}

void PS4Controller::hidEventHandler(void *arg, esp_event_base_t base, int32_t id, void *event_data) {
    auto *data = static_cast<esp_hidh_event_data_t *>(event_data);

    switch (id) {
        case ESP_HIDH_OPEN_EVENT:
            connected = true;
            ESP_LOGI(TAG, "Control PS4 conectado");
            break;
        case ESP_HIDH_BATTERY_EVENT:
            batteryLevel = data->battery.level;
            ESP_LOGI(TAG, "Batería: %d%%", batteryLevel);
            break;
        case ESP_HIDH_CLOSE_EVENT:
            connected = false;
            ESP_LOGI(TAG, "Control PS4 desconectado");
            break;
        case ESP_HIDH_INPUT_EVENT:
            ESP_LOGI(TAG, "Input recibido (%d bytes)", data->input.length);
            parseInput(data->input.data, data->input.length);
            break;
        default:
            ESP_LOGW(TAG, "Evento HID no manejado: %ld", id);
            break;
    }
}

void PS4Controller::gapCallback(esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t *param) {
    switch (event) {
        case ESP_BT_GAP_AUTH_CMPL_EVT:
            if (param->auth_cmpl.stat == ESP_BT_STATUS_SUCCESS) {
                ESP_LOGI(TAG, "Emparejamiento exitoso con %s", param->auth_cmpl.device_name);
            } else {
                ESP_LOGE(TAG, "Fallo de emparejamiento: %d", param->auth_cmpl.stat);
            }
            break;

        case ESP_BT_GAP_PIN_REQ_EVT:
            ESP_LOGI(TAG, "Solicitud de PIN, respondiendo 0000");
            esp_bt_pin_code_t pin_code;
            strcpy((char *)pin_code, "0000");
            esp_bt_gap_pin_reply(param->pin_req.bda, true, 4, pin_code);
            break;

        case ESP_BT_GAP_CFM_REQ_EVT:
            ESP_LOGI(TAG, "Confirmación SSP requerida, aceptando...");
            esp_bt_gap_ssp_confirm_reply(param->cfm_req.bda, true);
            break;

        case ESP_BT_GAP_DISC_RES_EVT: {
            esp_bt_gap_read_remote_name(param->disc_res.bda);
            memcpy(last_found_addr, param->disc_res.bda, sizeof(esp_bd_addr_t));
            break;
        }

        case ESP_BT_GAP_READ_REMOTE_NAME_EVT:
            if (param->read_rmt_name.stat == ESP_BT_STATUS_SUCCESS) {
                ESP_LOGI(TAG, "Nombre remoto: %s", param->read_rmt_name.rmt_name);
                if (strcmp((const char *)param->read_rmt_name.rmt_name, TARGET_NAME) == 0) {
                    ESP_LOGI(TAG, "¡Control PS4 detectado! Iniciando conexión...");
                    if (esp_bt_gap_cancel_discovery() == ESP_OK) {
                        ESP_LOGI(TAG, "Escaneo cancelado, conectando...");
                        esp_hidh_dev_t *dev = esp_hidh_dev_open(last_found_addr, ESP_HID_TRANSPORT_BT, 0);
                        if (!dev) {
                            ESP_LOGE(TAG, "Fallo al iniciar conexión HID");
                        }
                    }
                }
            }
            break;

        default:
            ESP_LOGI(TAG, "Evento GAP no manejado: %d", event);
            break;
    }
}

#include "MQTTManager.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "MQTTManager";

uint8_t MQTTManager::_rx_buffer[MQTTManager::BUFFER_SIZE] = {0};
int MQTTManager::_rx_buffer_head = 0;
int MQTTManager::_rx_buffer_tail = 0;

MQTTManager::MQTTManager()
    : _client(nullptr) {}

MQTTManager::~MQTTManager()
{
    stop();
}

void MQTTManager::setup(const char *uri, const char *username, const char *password)
{
    _brokerUri = uri;
    _username = username;
    _password = password;
}

bool MQTTManager::start()
{
    // esp_mqtt_client_config_t mqtt_cfg = {};
    // mqtt_cfg.uri = brokerUri.c_str();

    esp_mqtt_client_config_t mqtt_cfg = {};
    mqtt_cfg.broker.address.uri = _brokerUri;
    mqtt_cfg.credentials.username = _username;
    mqtt_cfg.credentials.authentication.password = _password;
    mqtt_cfg.broker.verification.certificate = isrg_root_x1_pem;

    _client = esp_mqtt_client_init(&mqtt_cfg);
    if (!_client)
    {
        ESP_LOGE(TAG, "Failed to initialize MQTT client.");
        return false;
    }

    esp_mqtt_client_register_event(_client, static_cast<esp_mqtt_event_id_t>(-1), mqttEventHandler, this);
    esp_err_t err = esp_mqtt_client_start(_client);

    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to start MQTT client: %s", esp_err_to_name(err));
        return false;
    }

    ESP_LOGI(TAG, "MQTT client started.");
    return true;
}

void MQTTManager::stop()
{
    if (_client)
    {
        esp_mqtt_client_stop(_client);
        esp_mqtt_client_destroy(_client);
        _client = nullptr;
        _connected = false;
    }
}

bool MQTTManager::isConnected() const
{
    return _connected;
}

void MQTTManager::publish(char *topic, char *message, size_t length)
{
    if (_client && _connected)
    {
        esp_mqtt_client_publish(_client, topic, message, length, 1, 0); //Penúltimo valor es el QoS
    }
    else
    {
        ESP_LOGW(TAG, "Cannot publish, not connected.");
    }
}

void MQTTManager::addAutoSubscribe(const std::string &topic) {
    _topics.push_back(topic);
}

void MQTTManager::subscribe(char *topic)
{
    if (_client && _connected)
    {
        esp_mqtt_client_subscribe(_client, topic, 1); //Último valor es el QoS
    }
    else
    {
        ESP_LOGW(TAG, "Cannot subscribe, not connected.");
    }
}

int MQTTManager::available()
{
    return (_rx_buffer_head >= _rx_buffer_tail)
               ? (_rx_buffer_head - _rx_buffer_tail)
               : (BUFFER_SIZE - _rx_buffer_tail + _rx_buffer_head);
}

int MQTTManager::read(char *buffer, size_t length)
{
    if (_rx_buffer_head == _rx_buffer_tail)
        return 0;

    size_t count = 0;
    while (count < length && _rx_buffer_head != _rx_buffer_tail)
    {
        buffer[count++] = _rx_buffer[_rx_buffer_tail];
        _rx_buffer_tail = (_rx_buffer_tail + 1) % BUFFER_SIZE;
    }
    return count;
}

void MQTTManager::mqttEventHandler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    MQTTManager *self = static_cast<MQTTManager *>(handler_args);
    esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t)event_data;
    self->handleEvent(event);
}

void MQTTManager::handleEvent(esp_mqtt_event_handle_t event)
{
    switch (event->event_id)
    {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT connected");
        _connected = true;
        for (const auto &topic : _topics) {
            esp_mqtt_client_subscribe(_client, topic.c_str(), 1);
            ESP_LOGI(TAG, "Subscribed to %s", topic.c_str());
        }        
        break;

    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGW(TAG, "MQTT disconnected");
        _connected = false;
        break;

    case MQTT_EVENT_DATA:
        for (int i = 0; i < event->data_len; ++i)
        {
            _rx_buffer[_rx_buffer_head] = event->data[i];
            _rx_buffer_head = (_rx_buffer_head + 1) % BUFFER_SIZE;
            if (_rx_buffer_head == _rx_buffer_tail)
            {
                _rx_buffer_tail = (_rx_buffer_tail + 1) % BUFFER_SIZE; // Overwrite oldest
            }
        }
        break;

    default:
        break;
    }
}

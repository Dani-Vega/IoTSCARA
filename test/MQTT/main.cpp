#include "definitions.h"

extern "C" void app_main()
{
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    esp_task_wdt_deinit();

    // Iniciar WiFi
    wifi.start();

    mqtt.addAutoSubscribe("ps4/control");
    mqtt.setup(uri, username, password);
    mqtt.start();

    while (1)
    {
        message_length = sprintf(message, "15.0,10.0,2.0\n");
        mqtt.publish("scara/status", message, message_length);
        message_length = mqtt.available();
        if (message_length)
        {
            mqtt.read(buffer, message_length); // Leer datos
            // Parseo tipo 1,12.0,45.0,30.0
            sscanf(buffer, "%d,%f,%f,%f\n", &modo, &t1, &t2, &t3);
        }
        printf("Datos Recibidos: %d, %.2f, %.2f, %.2f\n", modo, t1, t2, t3);
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

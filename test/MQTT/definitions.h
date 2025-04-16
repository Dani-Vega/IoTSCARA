#include <esp_err.h>
#include "nvs_flash.h"
#include "nvs.h"
#include <esp_log.h>

#include <esp_task_wdt.h>

#include <WiFiManager.h>
#include <MQTTManager.h>

const char* uri = "mqtts://da16513bd15d4ebe8c54b66e1be44028.s1.eu.hivemq.cloud:8883";

const char* username = "galvarez2004";
const char* password = "G.@.s.412004";

std::string ssid = "Totalplay-2.4G-25a0";
std::string password_wifi = "rECQkHteW4qZpDtY";

WiFiManager wifi(ssid, password_wifi, 1);
MQTTManager mqtt;

char buffer[128];
char message[128];
uint8_t message_length;

int modo;
float t1, t2, t3;
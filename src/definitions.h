#include <esp_err.h>
#include "nvs_flash.h"
#include "nvs.h"
#include <esp_log.h>

#include <OTAManager.h>
#include <WiFiManager.h>
#include <WebServer.h>

#include <TB6612.h>
#include <QuadratureEncoder.h>
#include <PID.h>
#include <MotorController.h>

#include <MotorStepper.h>
#include <AS5600.h>
#include <Kinematics.h>

#include <SimpleSerialBT.h>

// #include <esp_wpa2.h> // En vez de usar esta, usar la de abajo

// #include <esp_eap_client.h>

#include <esp_task_wdt.h>

#include <WiFiManager.h>
#include <MQTTManager.h>

#include <portmacro.h>

const char *uri = "mqtts://da16513bd15d4ebe8c54b66e1be44028.s1.eu.hivemq.cloud:8883";

const char *username = "galvarez2004";
const char *password = "G.@.s.412004";

std::string ssid = "Totalplay-2.4G-25a0";
std::string password_wifi = "rECQkHteW4qZpDtY";

WiFiManager wifi(ssid, password_wifi, 1);
MQTTManager mqtt;

char buffer_mqtt[128];
char message_mqtt[128];
uint8_t message_length_mqtt;

int modo;
float t1, t2, t3;

// Protección de datos compartidos
portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;

OTAManager ota;

MotorStepper j1_motor(DRIVER, 19, 23);
MotorStepper j2_motor(DRIVER, 25, 17);
MultiStepper XY;
Kinematics kinematics;
AS5600 encoder;

TB6612 j3_HBridge;
QuadratureEncoder j3_encoder;
PID j3_PID;
MotorController j3_motor(j3_HBridge, j3_encoder, j3_PID);

TB6612 j4_HBridge;
QuadratureEncoder j4_encoder;
PID j4_PID;
MotorController j4_motor(j4_HBridge, j4_encoder, j4_PID);

MultiDC Z;

SerialBT bt;

char buffer[128];
char message[200];
uint8_t message_length;

uint8_t j1_encoder_pins[2] = {21, 22}; // SDA, SCL

uint8_t j3_pins[2] = {27, 12};
uint8_t j3_channels[2] = {0, 1};
uint8_t j3_encoder_pins[2] = {14, 13};

uint8_t j4_pins[2] = {2, 4};
uint8_t j4_channels[2] = {2, 3};
uint8_t j4_encoder_pins[2] = {16, 15};

float degrees_per_edge = 360.0f / 4200; // CPR calculation
uint64_t timeout_ms = 200000;
float pid_gains_j3[3] = {1.1f, 0.0f, 0.01f};
float pid_gains_j4[3] = {1.0f, 0.0f, 0.01f};

uint64_t samplingTime = 250;

uint64_t current_time;
uint64_t prev_time_calculations;

// Driver 1 step = 19 dir = 23
// Driver 2 step = 25 dir = 17

int mode;

float gear_ratio[4] = {16, 3, 3.18 / 360, 1.83333};
float epsilon = 0.01; // tolerancia

float pose[6];
float joint_variables[4];

float prev_target_joint_variables[4];
float target_joint_variables[4];
float steppers_targets[2];
float dc_targets[2];

float _reference[4];

float prev_operational_var[4];
float operational_var[4];
float solutions[2][4];

float DH_Parameters[4][4] = {
    {0, 10, 0, 5},  // Desde la base hasta el primer stepper
    {60, 0, 0, 13}, //
    {50, 0, 180, 11},
    {30, 3, 0, 0}}; // Altura gripper y giro

// _DH[0][1] = joint_variables[2]; // d
// _DH[1][0] = joint_variables[0]; // θ1
// _DH[2][0] = joint_variables[1]; // θ2
// _DH[3][0] = joint_variables[3]; // θ3

// float x = operational_var[0];
// float y = operational_var[1];
// float z = operational_var[2];
// float theta_tool = operational_var[3];

// Solutions IK

// solutions[0][0] = RAD2DEG * theta1_1;
// solutions[0][1] = RAD2DEG * theta2_1;
// solutions[0][2] = z;
// solutions[0][3] = RAD2DEG * theta3_1;
// solutions[1][0] = RAD2DEG * theta1_2;
// solutions[1][1] = RAD2DEG * theta2_2;
// solutions[1][2] = z;
// solutions[1][3] = RAD2DEG * theta3_2;
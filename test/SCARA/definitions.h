#include <esp_err.h>
#include "nvs_flash.h"
#include "nvs.h"
#include <esp_log.h>

#include <RGB.h>
#include <Servo.h>
#include <DFPlayerMini.h>

#include <SimpleTimer.h>

#include <OTAManager.h>
#include <WiFiManager.h>
#include <WebServer.h>

#include <TB6612.h>
#include <QuadratureEncoder.h>
#include <PID.h>
#include <MotorController.h>

#include <SimpleSerialBT.h>

#include <esp_wpa2.h> // En vez de usar esta, usar la de abajo

#include <esp_eap_client.h>

RGB strips;
Servo servo;
DFPlayerMini speaker;
SimpleTimer timer;

OTAManager ota;

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

uint8_t j1_encoder_pins[2] = {21,22}; //SDA, SCL

uint8_t j3_pins[2] = {27,12};
uint8_t j3_channels[2] = {0,1};
uint8_t j3_encoder_pins[2] = {14,13};

uint8_t j4_pins[2] = {2,4};
uint8_t j4_channels[2] = {2,3};
uint8_t j4_encoder_pins[2] = {16,15};

float degrees_per_edge = 360.0f / 4200; // CPR calculation
uint64_t timeout_ms = 200000;
float pid_gains_j3[3] = {1.1f, 0.0f, 0.01f}; 
float pid_gains_j4[3] = {1.0f, 0.0f, 0.01f};

uint64_t samplingTime = 250;

uint64_t current_time;
uint64_t prev_time;
uint64_t prev_time_calculations;

uint64_t prev_print_time = 0;


void onTimer(void *arg)
{
    timer.setInterrupt();
}

//Driver 1 step = 19 dir = 23
//Driver 2 step = 25 dir = 17

int mode;

float gear_ratio[4] = {16, 3, 3.18/360, 1.83333};
float epsilon = 0.01;  // tolerancia

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

// _DH[0][1] = joint_variables[2]; // d
// _DH[1][0] = joint_variables[0]; // θ1
// _DH[2][0] = joint_variables[1]; // θ2
// _DH[3][0] = joint_variables[3]; // θ3

// float x = operational_var[0];
// float y = operational_var[1];
// float z = operational_var[2];
// float theta_tool = operational_var[3];
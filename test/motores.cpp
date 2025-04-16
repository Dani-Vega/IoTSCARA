#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <MotorStepper.h>
#include <AS5600.h>
#include <Kinematics.h>
#include "definitions.h"

extern "C" void app_main()
{
    esp_task_wdt_deinit();

    float position =  0.5 * (360*1.211111111); //Aprox 4cm
    j3_motor.setup(j3_pins, j3_channels, 25000, 8, 3.3, 2.343f, j3_encoder_pins, degrees_per_edge, timeout_ms, 0.01, 10);
    j3_motor.setPIDGains(pid_gains_j3);
    j3_motor.setControlMode(2);

    j4_motor.setup(j4_pins, j4_channels, 25000, 8, 3.3f, 2.409f, j4_encoder_pins, degrees_per_edge, timeout_ms, 0.01, 10);
    j4_motor.setPIDGains(pid_gains_j4);
    j4_motor.setControlMode(2);

    j4_motor.setReference(position);

    /*Z.addMotor(j3_motor);
    Z.addMotor(j4_motor);*/

    while (1)
    {
        current_time = esp_timer_get_time();

        if (current_time - prev_time >= 10000) // Procesar cada segundo
        {

                j4_motor.update();
            

            prev_time = current_time; // Actualizar el tiempo previo
        }

        if (current_time - prev_print_time >= 1000000) // Procesar cada segundo
        {
            printf("Angle: %.2f, Output: %.2f, Error: %.2f\n", j4_encoder.getCumulativeAngle(), j4_motor.getOutput(), j4_motor.getError());
            prev_print_time = current_time; // Actualizar el tiempo previo
        }
    }
}

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <MotorStepper.h>
#include <AS5600.h>
#include <Kinematics.h>
#include "definitions.h"

MotorStepper j1_motor(DRIVER, 25, 17);
MotorStepper j2_motor(DRIVER, 19, 23);
MultiStepper XY;
Kinematics kinematics;
AS5600 encoder;

extern "C" void app_main()
{
    esp_task_wdt_deinit();

    // Configurar motores
    j1_motor.setMaxSpeed(4000);
    j1_motor.setAcceleration(2000);

    j2_motor.setMaxSpeed(4000);
    j2_motor.setAcceleration(2000);

    XY.addStepper(j1_motor);
    XY.addStepper(j2_motor);

    j3_motor.setup(j3_pins, j3_channels, 25000, 8, 3.3, 2.343f, j3_encoder_pins, degrees_per_edge, timeout_ms, 0.01, 10);
    j3_motor.setPIDGains(pid_gains_j3);
    j3_motor.setControlMode(2);

    j4_motor.setup(j4_pins, j4_channels, 25000, 8, 3.3f, 2.409f, j4_encoder_pins, degrees_per_edge, timeout_ms, 0.01, 10);
    j4_motor.setPIDGains(pid_gains_j4);
    j4_motor.setControlMode(2);

    Z.addMotor(j3_motor);
    Z.addMotor(j4_motor);

    bt.begin("Hola"); // Inicialización del Bluetooth

    bt.getMacAddress();

    // Aquí Mover todos los motores hasta llegar a limit switch y al límite del piñón (quizá un 10%)

    // Configurar la cinemática con los parámetros DH del robot
    float DH_Parameters[4][4] = {
        {0, 10, 0, 5},  // Desde la base hasta el primer stepper
        {60, 0, 0, 13}, //
        {50, 0, 180, 11},
        {30, 3, 0, 0}}; // Altura gripper y giro
    kinematics.setup(DH_Parameters, 4);

    while (1)
    {
        current_time = esp_timer_get_time();

        // if (current_time - prev_time >= 62.5) // Procesar cada segundo
        // {
        //     j3_motor.update();
        //     j4_motor.update();
        //     prev_time = current_time; // Actualizar el tiempo previo
        // }

        if (current_time - prev_time >= 10000) // Procesar cada segundo
        {
            while(j1_motor.isRunning())
                j1_motor.run();
                
            j3_motor.update();
            j4_motor.update();

            prev_time = current_time;
        }

        if (current_time - prev_time_calculations >= 100000) // Procesar cada segundo
        {
            int64_t start_time = esp_timer_get_time();
            float joint_variables[4] = {
                // encoder.getCumulativePosition() / 16.0f,
                (j1_motor.currentPosition() * 360.0f) / 20000.0f,
                0.0,
                j3_encoder.getCumulativeAngle(),
                j4_encoder.getCumulativeAngle()};

            message_length = sprintf(message, "%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f%.2f,%.2f\n",
                                     joint_variables[0], joint_variables[1], joint_variables[2], joint_variables[3], 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0);
            bt.write(message, message_length);
            message_length = bt.available();
            if (message_length)
            {
                bt.read(buffer, message_length); // Leer datos del Bluetooth

                // Parsear los datos en el formato: mode, theta1, theta2, Z, X, Y, Zi (InverseK), theta3
                int result = sscanf(buffer, "%d,%f,%f,%f,%f,%f,%f,%f\n",
                                    &mode, &target_joint_variables[0], &target_joint_variables[1], &target_joint_variables[2], &operational_var[0], &operational_var[1], &operational_var[2], &target_joint_variables[3]); // Último es Yaw

                if (mode == 0) // Pick and place
                {
                    // Aquí implementas la lógica del pick and place
                    printf("Ejecutando Pick and Place...\n");
                }
                else if (mode == 1) // Cinemática Directa
                {
                    // Ejecutar cinemática directa, calcular la pose
                    printf("Ejecutando Cinemática Directa...\n");

                    bool areDifferent = false;
                    for (int i = 0; i < 4; i++)
                    {
                        if (target_joint_variables[i] != prev_target_joint_variables[i])
                        {
                            areDifferent = true;
                            break; // Si encontramos una diferencia, salimos del ciclo
                        }
                    }
                    if (areDifferent)
                        for (int i = 0; i < 2; ++i)
                        {
                            steppers_targets[i] = target_joint_variables[i];
                            dc_targets[i] = target_joint_variables[i + 2];

                            // XY.moveTo(steppers_targets);
                            j1_motor.moveToAbsolute((steppers_targets[0] / 360) * 200 * 25);

                            // Z.moveTo(dc_targets);

                            printf("theta1: %.2f, theta2: %.2f, Z: %.2f, theta3 = %f, Error3: %.2f, Error 4: %.2f, Reference: %.2f\n",
                                   joint_variables[0], joint_variables[1], j3_encoder.getCumulativeAngle(), joint_variables[3], j3_motor.getError(), j4_motor.getError(), (dc_targets[0]) * 360 / 3.18);

                            j3_motor.setReference((dc_targets[0]) * 360 / 3.18);
                            j4_motor.setReference(dc_targets[1] * 1.211111111);
                        }

                    else
                    {
                        j3_motor.setReference((dc_targets[0]) * 360 / 3.18);
                        j4_motor.setReference(dc_targets[1] * 1.211111111);
                    }
                }
                else if (mode == 2) // Cinemática Inversa
                {
                    // Ejecutar cinemática inversa, calcular las posiciones de las articulaciones
                    printf("Ejecutando Cinemática Inversa...\n");

                    bool areDifferent = false;
                    for (int i = 0; i < 4; i++)
                    {
                        if (operational_var[i] != prev_operational_var[i])
                        {
                            areDifferent = true;
                            break; // Si encontramos una diferencia, salimos del ciclo
                        }
                    }

                    int num_solutions = kinematics.calculateIK(operational_var, solutions);

                    // Seleccionar la mejor solución
                    int best_solution_index = kinematics.selectBestIK(solutions);

                    if (best_solution_index >= 0)
                    {
                        // Obtener la mejor solución
                        float best_solution[4];
                        for (int i = 0; i < 2; i++)
                        {
                            steppers_targets[i] = solutions[best_solution_index][i];
                            dc_targets[i] = target_joint_variables[i + 2];
                        }

                        // Asignar los valores de las articulaciones
                        printf("Mejor solución IK: Theta1 = %f, Theta2 = %f, Z = %f, Theta3 = %f\n",
                               steppers_targets[0], steppers_targets[1], dc_targets[2], dc_targets[3]);

                        // XY.moveTo(steppers_targets);
                        j1_motor.move(steppers_targets[0]);

                        // Z.moveTo(dc_targets);

                        // printf("Error: %.2f", (dc_targets[0]) * 360 / 3.18);
                        j3_motor.setReference((dc_targets[0]) * 360 / 3.18);
                        j4_motor.setReference(dc_targets[1] * 1.211111111);
                    }
                }

                for (int i = 0; i < 4; ++i)
                    prev_target_joint_variables[i] = target_joint_variables[i];
                for (int i = 0; i < 4; ++i)
                    prev_operational_var[i] = operational_var[i];

                printf("mode: %d\n", mode);
                printf("Theta1: %f, Theta2: %f, Theta3: %f, Z: %f\n", target_joint_variables[0], target_joint_variables[1], target_joint_variables[3], target_joint_variables[2]);
                printf("X: %f, Y: %f, Z: %f, Theta3: %f\n", operational_var[0], operational_var[1], operational_var[2], operational_var[3]);
                printf("Pos actual: %.2f, %.2f, %.2f, %.2f ", joint_variables[0], joint_variables[1], joint_variables[2], joint_variables[3]);
                printf("theta1: %.2f, theta2: %.2f, Z: %.2f, theta3 = %f, Error3: %.2f, Error 4: %.2f,\n",
                       joint_variables[0], joint_variables[1], joint_variables[2], joint_variables[3], j3_motor.getError(), j4_motor.getError());
            }

            prev_time_calculations = current_time;
        }
    }
}

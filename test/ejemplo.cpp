#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "MotorStepper.h"
#include "TB6612.h"
#include "Kinematics.h" // Asegúrate de que Kinematics esté disponible
#include "definitions.h"

MotorStepper motor(DRIVER, 25, 17);
Kinematics kinematics;

// Variables para recibir datos
float prev_theta1, prev_theta2, prev_Z, prev_X, prev_Y, prev_Zi, prev_theta3;
float current_theta1, current_theta2 = 45, current_theta3;

extern "C" void app_main()
{
    // Configurar motores
    motor.setMaxSpeed(4000);
    motor.setAcceleration(2000);

    motor2.setup(motor_pins, motor_channels, 25000, 8, 3.3f, 0.30f);

    bt.begin("Yaroar"); // Inicialización del Bluetooth

    // Configurar la cinemática con los parámetros DH del robot
    float DH_Parameters[4][4] = {/* Asigna tus parámetros DH aquí */};
    kinematics.setup(DH_Parameters, 4);

    while (1)
    {
        current_time = esp_timer_get_time();

        if (current_time - prev_time >= 1000000) // Procesar cada segundo
        {
            message_length = sprintf(message, "%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f\n", joint_variables[0], joint_variables[1], joint_variables[3], joint_variables[2], 1.0, 2.0, 3.0, 4.0, 5.5, 5.5);
            bt.write(message, message_length);
            message_length = bt.available();
            if (message_length)
            {
                bt.read(buffer, message_length); // Leer datos del Bluetooth

                // Parsear los datos en el formato: mode, theta1, theta2, Z, X, Y, Zi (InverseK), theta3
                int result = sscanf(buffer, "%d,%f,%f,%f,%f,%f,%f,%f\n",
                                    &mode, &target_joint_variables[0], &target_joint_variables[1], &target_joint_variables[2], &operational_var[0], &operational_var[1], &operational_var[2], &target_joint_variables[3], operational_var[3]); // Último es Yaw

                printf("mode: %d\n", mode);
                printf("Theta1: %f, Theta2: %f, Theta3: %f, Z: %f\n", target_joint_variables[0], target_joint_variables[1], target_joint_variables[3], target_joint_variables[2]);
                printf("X: %f, Y: %f, Z: %f, Theta3: %f\n", operational_var[0], operational_var[1], operational_var[2], operational_var[3]);

                // Ejecutar la lógica según el mode recibido
                if (mode == 0) // Pick and place
                {
                    // Aquí implementas la lógica del pick and place
                    printf("Ejecutando Pick and Place...\n");
                }
                else if (mode == 1) // Cinemática Directa
                {
                    // Ejecutar cinemática directa, calcular la pose
                    printf("Ejecutando Cinemática Directa...\n");

                    // Calcular la máscara de cambios
                    uint8_t changed_mask = 0;
                    for (int i = 0; i < 4; ++i)
                        changed_mask |= (fabs(operational_var[i] - prev_operational_var[i]) > epsilon) << i;
                    if (changed_mask & 0b00000011)
                    {
                        long targets[2];

                        for (int i = 0; i < 2; ++i)
                        {
                            float delta = operational_var[i] - prev_operational_var[i];
                            targets[i] = (long)(delta / 360.0 * steps_per_rev[i] * gear_ratio[i]);
                        }

                        multiStepper.moveTo(targets);
                        multiStepper.runSpeedToPosition(); // o el método que uses
                    }

                    if (changed_mask & 0b00001100) {
                        for (int i = 2; i < 4; ++i) {
                            if (changed_mask & (1 << i)) {
                                float delta = operational_var[i] - prev_operational_var[i];
                                int target_ticks = (int)(delta * dc_conversion_ratio[i - 2]);
                                dc_motor[i - 2].moveTo(target_ticks);  // función personalizada con PID
                            }
                        }
                    }
                    
                    float pose[6];
                    float joint_variables[4] = {theta1, theta2, Z, theta3};
                    kinematics.updateJointVariables(joint_variables);
                    kinematics.calculateFK(pose);

                    // Pose es el resultado, que contiene los valores de posición y orientación
                    printf("Pose calculada: X = %f, Y = %f, Z = %f, Roll = %f, Pitch = %f, Yaw = %f\n",
                           pose[3], pose[4], pose[5], pose[0], pose[1], pose[2]);
                }
                else if (mode == 2) // Cinemática Inversa
                {
                    // Ejecutar cinemática inversa, calcular las posiciones de las articulaciones
                    printf("Ejecutando Cinemática Inversa...\n");

                    float solutions[2][4];                        // Dos soluciones posibles
                    float operational_var[4] = {X, Y, Z, theta3}; // X, Y, Z, yaw (theta3)

                    int num_solutions = kinematics.calculateIK(operational_var, solutions);

                    // Seleccionar la mejor solución
                    int best_solution_index = kinematics.selectBestIK(solutions);

                    if (best_solution_index >= 0)
                    {
                        // Obtener la mejor solución
                        float best_solution[4];
                        for (int i = 0; i < 4; i++)
                            best_solution[i] = solutions[best_solution_index][i];

                        // Asignar los valores de las articulaciones
                        printf("Mejor solución IK: Theta1 = %f, Theta2 = %f, Z = %f, Theta3 = %f\n",
                               best_solution[0], best_solution[1], best_solution[2], best_solution[3]);
                    }
                }
            }

            while (motor.isRunning())
            {
                current_theta2 = (motor.currentPosition() / 400) * 360 / 3;
                motor.run(); // Ejecutar el movimiento
            }

            for (int i = 0; i < 4; ++i)
                prev_target_joint_variables[i] = target_joint_variables[i];
            for (int i = 0; i < 4; ++i)
                prev_operational_var[i] = operational_var[i];

            prev_time = current_time; // Actualizar el tiempo previo
        }
    }
}

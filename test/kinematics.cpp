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
int modo = 0;
float theta1, theta2, Z, X, Y, Zi, theta3;
float prev_theta1, prev_theta2, prev_Z, prev_X, prev_Y, prev_Zi, prev_theta3;
float current_theta1, current_theta2 = 45, current_theta3;
float pid_gains[3];
int samplingPeriod_motor = 1000; // Valor inicial

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

                // Parsear los datos en el formato: modo, theta1, theta2, Z, X, Y, Zi (InverseK), theta3
                int result = sscanf(buffer, "%d,%f,%f,%f,%f,%f,%f,%f\n",
                                    &modo, &target_joint_variables[0], &target_joint_variables[1], &target_joint_variables[2], &operational_var[0], &operational_var[1], &operational_var[2], &target_joint_variables[3], operational_var[3]); //Último es Yaw

                printf("Modo: %d\n", modo);
                printf("Theta1: %f, Theta2: %f\n", theta1, theta2);
                printf("Zi: %f, X: %f, Y: %f\n", Zi, X, Y);
                printf("Z: %f, Theta3: %f\n", Z, theta3);

                // Ejecutar la lógica según el modo recibido
                if (modo == 0) // Pick and place
                {
                    // Aquí implementas la lógica del pick and place
                    printf("Ejecutando Pick and Place...\n");
                }
                else if (modo == 1) // Cinemática Directa
                {
                    // Ejecutar cinemática directa, calcular la pose
                    printf("Ejecutando Cinemática Directa...\n");

                    // Calcular la diferencia de posiciones
                    if (theta1 != prev_theta1 || theta2 != prev_theta2 || theta3 != prev_theta3)
                    {
                        // Convertir la diferencia de ángulo a pasos de motor
                        int steps_theta1 = (int)((theta1 - prev_theta1) / 360 * 400 * 3);
                        int steps_theta2 = (int)((theta2 - prev_theta2) / 360 * 400 * 3);
                        int steps_theta3 = (int)((theta3 - prev_theta3) / 360 * 400 * 3);

                        motor.moveTo(steps_theta2);
                    }
                    float pose[6];
                    float joint_variables[4] = {theta1, theta2, Z, theta3};
                    kinematics.updateJointVariables(joint_variables);
                    kinematics.calculateFK(pose);

                    // Pose es el resultado, que contiene los valores de posición y orientación
                    printf("Pose calculada: X = %f, Y = %f, Z = %f, Roll = %f, Pitch = %f, Yaw = %f\n",
                           pose[3], pose[4], pose[5], pose[0], pose[1], pose[2]);
                }
                else if (modo == 2) // Cinemática Inversa
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
                current_theta2  = (motor.currentPosition() / 400) * 360 / 3;
                motor.run(); // Ejecutar el movimiento
            }

            prev_theta1 = theta1;
            prev_theta2 = theta2;
            prev_Z = Z;
            prev_X = X;
            prev_Y = Y;
            prev_Zi = Zi;
            prev_theta3 = theta3;
            prev_time = current_time; // Actualizar el tiempo previo
        }
    }
}

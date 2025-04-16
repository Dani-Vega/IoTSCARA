#include "definitions.h"

void processing_task(void *pvParameters)
{
    while (1)
    {
        //UBaseType_t watermark = uxTaskGetStackHighWaterMark(NULL);
        //printf("Watermark Process: %u\n", watermark);
        current_time = esp_timer_get_time();
        if (current_time - prev_time_calculations >= 100000)
        {
            float joint_variables[4] = {
                // encoder.getCumulativePosition() / 16.0f,
                (j1_motor.currentPosition() * 360.0f) / 5000.0f,
                (j2_motor.currentPosition() * 360.0f) / 600.0f,
                j3_encoder.getCumulativeAngle(),
                j4_encoder.getCumulativeAngle()};

            // MQTT
            message_length_mqtt = sprintf(message_mqtt, "15.0,10.0,2.0\n");
            mqtt.publish("scara/status", message_mqtt, message_length_mqtt);
            message_length_mqtt = mqtt.available();
            if (message_length_mqtt)
            {
                mqtt.read(buffer_mqtt, message_length_mqtt); // Leer datos
                // Parseo tipo 1,12.0,45.0,30.0
                sscanf(buffer_mqtt, "%d,%f,%f,%f", &modo, &t1, &t2, &t3);
            }
            printf("Datos Recibidos: %d, %.2f, %.2f, %.2f\n", modo, t1, t2, t3);

            // Bluetooth
            message_length = sprintf(message, "%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f%.2f,%.2f\n",
                                     joint_variables[0], joint_variables[1], joint_variables[2], joint_variables[3], 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0);
            bt.write(message, message_length);
            message_length = bt.available();
            if (message_length)
            {
                bt.read(buffer, message_length); // Leer datos del Bluetooth

                // Parsear los datos en el formato: mode, theta1, theta2, Z, X, Y, Zi (InverseK), theta3
                int result = sscanf(buffer, "%d,%f,%f,%f,%f,%f,%f,%f,%f\n",
                                    &mode, &target_joint_variables[0], &target_joint_variables[1], &target_joint_variables[2], &operational_var[0], &operational_var[1], &operational_var[2], &operational_var[3], &target_joint_variables[3]); // Último es Yaw

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

                    if (areDifferent)

                        if (best_solution_index >= 0)
                        {
                            for (int i = 0; i < 2; i++)
                            {
                                steppers_targets[i] = solutions[best_solution_index][i];
                                dc_targets[i] = solutions[best_solution_index][i + 2];
                            }

                            // Asignar los valores de las articulaciones
                            printf("Mejor solución IK: Theta1 = %f, Theta2 = %f, Z = %f, Theta3 = %f\n",
                                   steppers_targets[0], steppers_targets[1], dc_targets[2], dc_targets[3]);
                        }
                }

                portENTER_CRITICAL(&mux);
                // XY.moveTo(steppers_targets);
                j1_motor.moveToAbsolute((steppers_targets[0] / 360) * 5000);
                j2_motor.moveToAbsolute((steppers_targets[1] / 360) * 600);

                j3_motor.setReference((dc_targets[0]) * 360 / 3.18);
                j4_motor.setReference(dc_targets[1] * 1.211111111);
                portEXIT_CRITICAL(&mux);

                for (int i = 0; i < 4; ++i)
                    prev_target_joint_variables[i] = target_joint_variables[i];
                for (int i = 0; i < 4; ++i)
                    prev_operational_var[i] = operational_var[i];
            }
            prev_time_calculations = current_time;
        }
        // vTaskDelay(pdMS_TO_TICKS(500)); // Para pruebas (delay largo bloqueante en este núcleo)
    }
}

void motion_task(void *pvParameters)
{
    while (true)
    {
        //UBaseType_t watermark = uxTaskGetStackHighWaterMark(NULL);
        //printf("Watermark Motors: %u\n", watermark);
        //printf("Moviendo motores\n");
        if (j1_motor.isRunning())
            j1_motor.run();
        if (j2_motor.isRunning())
            j2_motor.run();

        j3_motor.update();
        j4_motor.update();
    }
}

extern "C" void app_main()
{
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    esp_task_wdt_deinit();

    // Iniciar WiFi
    wifi.start();

    // Iniciar MQTT
    mqtt.addAutoSubscribe("ps4/control");
    mqtt.setup(uri, username, password);
    mqtt.start();

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

    kinematics.setup(DH_Parameters, 4);

    // Crear tareas en diferentes núcleos
    xTaskCreatePinnedToCore(processing_task, "ProcessingTask", 8192, NULL, 1, NULL, 0); // Núcleo 0
    xTaskCreatePinnedToCore(motion_task, "MotionTask", 4096, NULL, 1, NULL, 1);         // Núcleo 1
}

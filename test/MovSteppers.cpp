#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "MotorStepper.h"

extern "C" void app_main()
{
    // Crear una instancia del motor
    MotorStepper motor(DRIVER, 19, 23);
    // Configurar velocidad y aceleración
    motor.setMaxSpeed(4000); // 16000 (Para el primer link)
    motor.setAcceleration(2000); // 8000 (Para el primer link)

    while (1)
    {
        // Mover el motor hacia adelante
        motor.move(600); // Mover 600 pasos hacia adelante (90° - segundo link)
        while (motor.isRunning())
        {
            motor.run(); // Ejecutar el movimiento
            printf("Pos Motor: %d\n", (motor.currentPosition()*360)/2400);  // 360/20000 (Para primer link)
            printf("Vel Motor: %d\n", motor.speed());
        }
        vTaskDelay(1000 / portTICK_PERIOD_MS); // Espera de 1 segundo

        // Mover el motor hacia atrás
        motor.move(-600); // Mover 600 pasos hacia atrás (90° - segundo link)
        while (motor.isRunning())
        {
            motor.run(); // Ejecutar el movimiento
            printf("Pos Motor: %d\n", (motor.currentPosition()*360)/2400); 
            printf("Vel Motor: %d\n", motor.speed());
        }

        vTaskDelay(1000 / portTICK_PERIOD_MS); // Espera de 1 segundo
    }
}

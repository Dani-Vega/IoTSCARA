#ifndef __Serial__H__
#define __Serial__H__

#include <stdio.h>
#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "driver/uart.h"
#include "string.h"
#include "driver/gpio.h"

class Serial
{
public:
    Serial();
    ~Serial();
    void setup(int baudrate, int txd, int rxd);
    void end(void);
    int available(void);
    uint8_t read(void);

    void print(char *ch);
    void println(char *ch);
    void write(uint8_t ch);
    void write_buffer(uint8_t *ch, int len);

private:
#define RX_BUF_SIZE 1024
#define TX_BUF_SIZE 1024

    typedef struct
    {
        TaskHandle_t parent_task_handle;
        TaskHandle_t tx_task_handle;
        uint8_t tx_buf[TX_BUF_SIZE];
        int tx_len;
        TaskHandle_t rx_task_handle;
        uint8_t rx_buf[RX_BUF_SIZE];
        int rx_len;
        int rx_save_idx;
        int rx_read_idx;
    } uart_obj_t;

    static uart_obj_t uart_obj;

    static void tx_task(void *pvParameters);
    static void rx_task(void *pvParameters);

    uint8_t peek(void);

    void flash();
};

#endif //__Serial__H__

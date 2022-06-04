/* UART Echo Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "sdkconfig.h"

/**
 * This is an example which echos any data it receives on configured UART back to the sender,
 * with hardware flow control turned off. It does not use UART driver event queue.
 *
 * - Port: configured UART
 * - Receive (Rx) buffer: on
 * - Transmit (Tx) buffer: off
 * - Flow control: off
 * - Event queue: off
 * - Pin assignment: see defines below (See Kconfig)
 */

#define ECHO_TEST_TXD (CONFIG_EXAMPLE_UART_TXD)
#define ECHO_TEST_RXD (CONFIG_EXAMPLE_UART_RXD)
#define ECHO_TEST_RTS (UART_PIN_NO_CHANGE)
#define ECHO_TEST_CTS (UART_PIN_NO_CHANGE)

// #define ECHO_UART_PORT_NUM      (CONFIG_EXAMPLE_UART_PORT_NUM)
#define ECHO_UART_PORT_NUM      (0)
#define ECHO_UART_PORT_NUM_IN   (1)
// #define ECHO_UART_BAUD_RATE     (CONFIG_EXAMPLE_UART_BAUD_RATE)
#define ECHO_UART_BAUD_RATE     (9600)
#define ECHO_TASK_STACK_SIZE    (CONFIG_EXAMPLE_TASK_STACK_SIZE)

#define BUF_SIZE (1024)

// #define BLINK_GPIO 5
// static int s_led_state = 0;

// static void blink_led(void)
// {
//     /* Set the GPIO level according to the state (LOW or HIGH)*/
//     gpio_set_level(BLINK_GPIO, s_led_state);
// }

// static void configure_led(void)
// {
//     // ESP_LOGI(TAG, "Example configured to blink GPIO LED!");
//     gpio_reset_pin(BLINK_GPIO);
//     /* Set the GPIO as a push/pull output */
//     gpio_set_direction(BLINK_GPIO, GPIO_MODE_OUTPUT);
// }

static int received = 0;

static void intOut(const char* desc, int n) {
    char buf[128];
    const char* text = (desc == NULL) ? "" : desc;
    sprintf(buf, "%s: %d\r", text, n);
    uart_write_bytes(ECHO_UART_PORT_NUM, buf, strlen(buf));
}

static void echo_task(void *arg)
{
    /* Configure parameters of an UART driver,
     * communication pins and install the driver */
    uart_config_t uart_config = {
        .baud_rate = ECHO_UART_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_APB,
    };
    int intr_alloc_flags = 0;

#if CONFIG_UART_ISR_IN_IRAM
    intr_alloc_flags = ESP_INTR_FLAG_IRAM;
#endif

    QueueHandle_t uart_queue, uart_queue_in;

    ESP_ERROR_CHECK(uart_driver_install(ECHO_UART_PORT_NUM, BUF_SIZE * 2, BUF_SIZE * 2, 10, &uart_queue, intr_alloc_flags));
    // ESP_ERROR_CHECK(uart_driver_install(ECHO_UART_PORT_NUM, BUF_SIZE * 2, 0, 0, NULL, intr_alloc_flags));
    ESP_ERROR_CHECK(uart_param_config(ECHO_UART_PORT_NUM, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(ECHO_UART_PORT_NUM, ECHO_TEST_TXD, ECHO_TEST_RXD, ECHO_TEST_RTS, ECHO_TEST_CTS));

    // ESP_ERROR_CHECK(uart_driver_install(ECHO_UART_PORT_NUM_IN, BUF_SIZE * 2, BUF_SIZE * 2, 10, &uart_queue_in, intr_alloc_flags));
    // // ESP_ERROR_CHECK(uart_driver_install(ECHO_UART_PORT_NUM, BUF_SIZE * 2, 0, 0, NULL, intr_alloc_flags));
    // ESP_ERROR_CHECK(uart_param_config(ECHO_UART_PORT_NUM_IN, &uart_config));
    // ESP_ERROR_CHECK(uart_set_pin(ECHO_UART_PORT_NUM_IN, ECHO_TEST_TXD, ECHO_TEST_RXD, ECHO_TEST_RTS, ECHO_TEST_CTS));


    // while(1) uart_write_bytes(ECHO_UART_PORT_NUM, "abcde", 6);

    // // Configure a temporary buffer for the incoming data
    // uint8_t *data = (uint8_t *) malloc(BUF_SIZE);
    static uint8_t data[BUF_SIZE];
    data[0] = 64;

    // configure_led();

    while (1) {
        // blink_led();
        // s_led_state = !s_led_state; 
        // s_led_state = 1;
        // Read data from the UART
        int len = uart_read_bytes(ECHO_UART_PORT_NUM, data, BUF_SIZE, 2000 / portTICK_RATE_MS);
        
        intOut(" > len", len);
        // if(len == 0) {
        //     printf("Empty imput\n");
        // }
        
        intOut(" > data[0]", data[0]);
        // if(len > 0) {
        //     uart_write_bytes(ECHO_UART_PORT_NUM, (const char *) data, len);
        // } else {
        //     uart_write_bytes(ECHO_UART_PORT_NUM, "Nothing!\n\r", 11);
        // }
        
    }
}

void app_main(void)
{
    xTaskCreate(echo_task, "uart_echo_task", ECHO_TASK_STACK_SIZE, NULL, 10, NULL);
}

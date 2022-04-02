#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/message_buffer.h"
#include <stdio.h>
#include <stdlib.h>

#define MAX_URL_SIZE 256


void configTask(void *args);


MessageBufferHandle_t urlMsg;
TaskHandle_t configTaskHandler;


void app_main(void)
{
    urlMsg = xMessageBufferCreate(MAX_URL_SIZE);
    // setvbuf(stdin, NULL, _IONBF, 0);
    // setvbuf(stdout, NULL, _IONBF, 0);
    BaseType_t configResult = xTaskCreate(configTask, NULL, 1024, NULL, 10, &configTaskHandler);

    // while(1) {}
}

void configTask(void *args) {
    int a = 0;
    const TickType_t ccc = 1000 / portTICK_PERIOD_MS;
    while(1) {
        printf("Hello: !\n");
        // scanf("%d", &a);
        vTaskDelay(ccc);
    }
}

// xTaskCreate( messageTask, NULL, configMINIMAL_STACK_SIZE+32 , NULL, 1, &taskMessageHandler );

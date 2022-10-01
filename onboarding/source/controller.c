#include "controller.h"
#include "amb_light_service.h"
#include "serial_io.h"

#include <FreeRTOS.h>
#include <os_task.h>
#include <os_timer.h>
#include <sys_common.h>

#include <gio.h>
#include <sci.h>

#include <string.h>

/* Declare handlers for tasks and timers */
static TaskHandle_t controllerTaskHandle = NULL;
static TimerHandle_t lightTimerHandle = NULL;
static TimerHandle_t ledTimerHandle = NULL;

/**
 * @brief Task that starts the led and light timers.
 * @param pvParameters Task parameters
 */
static void controllerTask(void * pvParameters);

/**
 * @brief The light timer callback function that sends a MEASURE_LIGHT event to the light service queue.
 */
static void lightTimerCallback(TimerHandle_t xTimer);

/**
 * @brief The led timer callback function that toggles the LED.
 */
static void ledTimerCallback(TimerHandle_t xTimer);

uint8_t initController(void) {
    BaseType_t xReturned = pdFAIL;
    if (controllerTaskHandle == NULL) {
        // Create controller task
        xReturned = xTaskCreate(controllerTask,             /* Function that implements the task. */
                                CONTROLLER_NAME,            /* Text name for the task. */
                                CONTROLLER_STACK_SIZE,      /* Stack size in words, not bytes. */
                                NULL,                       /* Parameter passed into the task. */
                                CONTROLLER_PRIORITY,        /* Priority at which the task is created. */
                                &controllerTaskHandle);     /* Used to pass out the created task's handle. */
    }

    if (ledTimerHandle == NULL) {
        // Create led timer
        ledTimerHandle = xTimerCreate(LED_TIMER_NAME,
                                    LED_TIMER_PERIOD,
                                    LED_TIMER_AUTORELOAD,
                                    (void *) 0,
                                    ledTimerCallback);
    }

    /* USER CODE BEGIN */
    // Create light timer and check if task/timers were created successfully
    if (lightTimerHandle == NULL) {
        // Create led timer
        lightTimerHandle = xTimerCreate(LIGHT_TIMER_NAME,
                                    LIGHT_TIMER_PERIOD,
                                    LIGHT_TIMER_AUTORELOAD,
                                    (void *) 0,
                                    lightTimerCallback);
    }

    if(xReturned == pdFAIL || ledTimerHandle == NULL || lightTimerHandle == NULL) {
        unsigned char* error_message = (unsigned char*)"Could not set up timer!";
        sciPrintText(scilinREG, (unsigned char *)error_message, strlen((char *)error_message));
    }
    /* USER CODE END */

    return 1;
}

static void controllerTask(void * pvParameters) {
    ASSERT(controllerTaskHandle != NULL);
    ASSERT(ledTimerHandle != NULL);
    ASSERT(lightTimerHandle != NULL);

    uint8_t lightServiceStatus = initLightService();
    if (lightServiceStatus == 0) {
        /* USER CODE BEGIN */
        // Deal with error when initializing light service task and/or queue

        /* USER CODE END */
    } else { 
        /* Light service task and queue created successfully */
        BaseType_t xReturnedLed = xTimerStart(ledTimerHandle, 0);
        
        /* USER CODE BEGIN */
        // Start light timer
        BaseType_t xReturnedLight = xTimerStart(lightTimerHandle, 0);

        if(xReturnedLed == pdFAIL || xReturnedLight == pdFAIL) {
            unsigned char* error_message = (unsigned char*)"Could not start timer!";
            sciPrintText(scilinREG, (unsigned char *)error_message, strlen((char *)error_message));
        }
        /* USER CODE END */
    }

    while (1);
}

static void ledTimerCallback(TimerHandle_t xTimer) {
    ASSERT(xTimer != NULL);

    gioToggleBit(gioPORTB, 1);
}

static void lightTimerCallback(TimerHandle_t xTimer) {
    ASSERT(xTimer != NULL);
    /* USER CODE BEGIN */
    // Send light event to light service queue
    light_event_t *event = MEASURE_LIGHT;
    sendToLightServiceQueue(event);
    /* USER CODE END */
}


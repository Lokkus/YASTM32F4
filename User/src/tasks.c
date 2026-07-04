#include "tasks.h"

#include "main.h"
#include "usb_device.h"
#include "usb_stdout.h"

#include "cmsis_os2.h"

#include <stdio.h>

/* -------------------------------------------------------------------------- */
/* Konfiguracja tasków                                                        */
/* -------------------------------------------------------------------------- */

#define TASK_SYSTEM_LOGGER_STACK_SIZE       512U
#define TASK_SYSTEM_LOGGER_PRIORITY         osPriorityLow

#define TASK_USB_DEVICE_STACK_SIZE          512U
#define TASK_USB_DEVICE_PRIORITY            osPriorityNormal

#define TASK_SYSTEM_LOGGER_START_DELAY_MS   1500U
#define TASK_SYSTEM_LOGGER_PERIOD_MS        1000U

/* -------------------------------------------------------------------------- */
/* Deklaracje prywatne                                                        */
/* -------------------------------------------------------------------------- */

static void task_system_logger(void *argument);
static void task_usb_device(void *argument);

/* -------------------------------------------------------------------------- */
/* Uchwyty tasków                                                             */
/* -------------------------------------------------------------------------- */

static osThreadId_t task_system_logger_handle;
static osThreadId_t task_usb_device_handle;

/* -------------------------------------------------------------------------- */
/* Atrybuty tasków                                                            */
/* -------------------------------------------------------------------------- */

static const osThreadAttr_t task_system_logger_attributes = {
    .name = "task_system_logger",
    .stack_size = TASK_SYSTEM_LOGGER_STACK_SIZE,
    .priority = TASK_SYSTEM_LOGGER_PRIORITY
};

static const osThreadAttr_t task_usb_device_attributes = {
    .name = "task_usb_device",
    .stack_size = TASK_USB_DEVICE_STACK_SIZE,
    .priority = TASK_USB_DEVICE_PRIORITY
};

/* -------------------------------------------------------------------------- */
/* Publiczne API                                                              */
/* -------------------------------------------------------------------------- */

void Tasks_Init(void)
{
    task_usb_device_handle = osThreadNew(
        task_usb_device,
        NULL,
        &task_usb_device_attributes
    );

    task_system_logger_handle = osThreadNew(
        task_system_logger,
        NULL,
        &task_system_logger_attributes
    );

    (void)task_usb_device_handle;
    (void)task_system_logger_handle;
}

/* -------------------------------------------------------------------------- */
/* Taski                                                                      */
/* -------------------------------------------------------------------------- */

static void task_system_logger(void *argument)
{
    (void)argument;

    osDelay(TASK_SYSTEM_LOGGER_START_DELAY_MS);

    USB_STDOUT_Init();

    for (;;)
    {
        printf("[task_system_logger] hello from task\r\n");

        osDelay(TASK_SYSTEM_LOGGER_PERIOD_MS);
    }
}

static void task_usb_device(void *argument)
{
    (void)argument;

    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_12, GPIO_PIN_SET);

    MX_USB_DEVICE_Init();

    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_14, GPIO_PIN_SET);

    osThreadExit();
}

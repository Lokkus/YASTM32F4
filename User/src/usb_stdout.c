#include "usb_stdout.h"

#include "cmsis_os2.h"
#include "usbd_cdc_if.h"
#include "usbd_def.h"

#include <stdint.h>
#include <stdio.h>

#define USB_STDOUT_TX_TIMEOUT_MS       100U
#define USB_STDOUT_TX_RETRY_DELAY_MS   1U

static volatile uint8_t usb_stdout_enabled = 0U;

static void usb_stdout_delay_1ms(void)
{
    if (osKernelGetState() == osKernelRunning)
    {
        osDelay(USB_STDOUT_TX_RETRY_DELAY_MS);
    }
}

void USB_STDOUT_Init(void)
{

    setvbuf(stdout, NULL, _IONBF, 0);

    usb_stdout_enabled = 1U;
}

void USB_STDOUT_DeInit(void)
{
    usb_stdout_enabled = 0U;
}

int __io_putchar(int ch)
{
    if (usb_stdout_enabled == 0U)
    {
        return ch;
    }

    uint8_t byte = (uint8_t)ch;
    uint32_t waited_ms = 0U;
    uint8_t result;

    do
    {
        result = CDC_Transmit_FS(&byte, 1U);

        if (result == USBD_OK)
        {
            usb_stdout_delay_1ms();
            return ch;
        }

        usb_stdout_delay_1ms();
        waited_ms += USB_STDOUT_TX_RETRY_DELAY_MS;

    } while (waited_ms < USB_STDOUT_TX_TIMEOUT_MS);

    return EOF;
}
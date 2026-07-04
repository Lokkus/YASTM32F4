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
    /*
     * Ten moduł zakładamy na razie tylko do użycia z tasków RTOS,
     * nie z przerwań.
     */
    if (osKernelGetState() == osKernelRunning)
    {
        osDelay(USB_STDOUT_TX_RETRY_DELAY_MS);
    }
}

void USB_STDOUT_Init(void)
{
    /*
     * Wyłączamy buforowanie stdout.
     *
     * Dzięki temu printf("abc\r\n") będzie od razu próbował wysłać dane,
     * a nie trzymał ich w buforze biblioteki C.
     */
    setvbuf(stdout, NULL, _IONBF, 0);

    usb_stdout_enabled = 1U;
}

void USB_STDOUT_DeInit(void)
{
    usb_stdout_enabled = 0U;
}

/*
 * Ta funkcja jest wołana przez _write() z Core/Src/syscalls.c.
 *
 * U Ciebie syscalls.c ma:
 *
 *   extern int __io_putchar(int ch) __attribute__((weak));
 *
 * oraz:
 *
 *   _write(...) { __io_putchar(...); }
 *
 * Dlatego wystarczy, że tutaj dostarczymy własną definicję __io_putchar().
 */
int __io_putchar(int ch)
{
    if (usb_stdout_enabled == 0U)
    {
        /*
         * Udajemy sukces, żeby printf() nie blokował się ani nie zwracał błędu,
         * jeśli ktoś przypadkiem zrobi printf przed inicjalizacją USB stdout.
         */
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
            /*
             * CDC_Transmit_FS() tylko zleca transmisję.
             * Ten mały delay daje stosowi USB czas na obsłużenie pakietu.
             *
             * Do pierwszego testu printf() jest OK.
             * Docelowo logger zrobimy na kolejce i callbacku TX complete.
             */
            usb_stdout_delay_1ms();
            return ch;
        }

        usb_stdout_delay_1ms();
        waited_ms += USB_STDOUT_TX_RETRY_DELAY_MS;

    } while (waited_ms < USB_STDOUT_TX_TIMEOUT_MS);

    return EOF;
}
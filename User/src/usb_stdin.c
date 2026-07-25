#include "usb_stdin.h"

#include "cmsis_os2.h"

#define USB_STDIN_QUEUE_CAPACITY 256U

static osMessageQueueId_t usb_stdin_queue;

void USB_STDIN_Init(void)
{
    usb_stdin_queue = osMessageQueueNew(
        USB_STDIN_QUEUE_CAPACITY,
        sizeof(uint8_t),
        NULL
    );
}

void USB_STDIN_PutRxData(const uint8_t *data, size_t length)
{
    if ((usb_stdin_queue == NULL) || (data == NULL))
    {
        return;
    }

    for (size_t i = 0U; i < length; ++i)
    {
        (void)osMessageQueuePut(usb_stdin_queue, &data[i], 0U, 0U);
    }
}

int USB_STDIN_GetChar(uint8_t *byte, uint32_t timeout_ms)
{
    if ((usb_stdin_queue == NULL) || (byte == NULL))
    {
        return 0;
    }

    return (osMessageQueueGet(usb_stdin_queue, byte, NULL, timeout_ms) == osOK);
}

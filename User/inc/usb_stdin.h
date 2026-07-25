#pragma once

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void USB_STDIN_Init(void);
void USB_STDIN_PutRxData(const uint8_t *data, size_t length);
int USB_STDIN_GetChar(uint8_t *byte, uint32_t timeout_ms);

#ifdef __cplusplus
}
#endif

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Retarget stdout na USB CDC.
 *
 * Po wywołaniu USB_STDOUT_Init() funkcje typu:
 *
 *   printf(...)
 *
 * będą szły przez:
 *
 *   printf()
 *     -> _write()
 *        -> __io_putchar()
 *           -> CDC_Transmit_FS()
 */

void USB_STDOUT_Init(void);
void USB_STDOUT_DeInit(void);

#ifdef __cplusplus
}
#endif
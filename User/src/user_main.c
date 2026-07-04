#include "app.h"
#include "bsp.h"

#include "cmsis_os2.h"

int main(void)
{
    BSP_Init();

    osKernelInitialize();

    APP_Init();

    osKernelStart();

    while (1)
    {
    }
}

#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    uint16_t year;
    uint8_t month;
    uint8_t day;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
} RTC_ClockDateTime;

void RTC_CLOCK_Init(void);
int RTC_CLOCK_SetDateTime(const RTC_ClockDateTime *date_time);
int RTC_CLOCK_GetDateTime(RTC_ClockDateTime *date_time);
int RTC_CLOCK_IsConfigured(void);
const char *RTC_CLOCK_SourceName(void);

#ifdef __cplusplus
}
#endif

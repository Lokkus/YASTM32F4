#include "rtc_clock.h"

#include "main.h"

#define RTC_CLOCK_BACKUP_MARKER_REG       RTC_BKP_DR0
#define RTC_CLOCK_BACKUP_MARKER_VALUE     0x5244434CU
#define RTC_CLOCK_LSE_TIMEOUT_MS          2000U

typedef enum
{
    RTC_CLOCK_SOURCE_NONE = 0,
    RTC_CLOCK_SOURCE_LSE,
    RTC_CLOCK_SOURCE_LSI
} RTC_ClockSource;

static RTC_HandleTypeDef hrtc;
static RTC_ClockSource rtc_clock_source = RTC_CLOCK_SOURCE_NONE;

static int rtc_clock_start_lse(void);
static int rtc_clock_configure_lse(void);
static int rtc_clock_configure_lsi(void);
static void rtc_clock_reset_backup_domain(void);
static void rtc_clock_configure_handle(RTC_ClockSource source);
static int rtc_clock_validate(const RTC_ClockDateTime *date_time);
static uint8_t rtc_clock_weekday(uint16_t year, uint8_t month, uint8_t day);
static uint8_t rtc_clock_days_in_month(uint16_t year, uint8_t month);
static int rtc_clock_is_leap_year(uint16_t year);

void RTC_CLOCK_Init(void)
{
    __HAL_RCC_PWR_CLK_ENABLE();
    HAL_PWR_EnableBkUpAccess();

    uint32_t rtc_source = __HAL_RCC_GET_RTC_SOURCE();

    if (rtc_source == RCC_RTCCLKSOURCE_LSE)
    
    {
        if (rtc_clock_start_lse() == 0)
        {
            rtc_clock_source = RTC_CLOCK_SOURCE_LSE;
        }
        else
        {
            rtc_clock_reset_backup_domain();

            if (rtc_clock_configure_lsi() == 0)
            {
                rtc_clock_source = RTC_CLOCK_SOURCE_LSI;
            }
        }
    }
    else if (rtc_source == RCC_RTCCLKSOURCE_LSI)
    {
        if (rtc_clock_configure_lsi() == 0)
        {
            rtc_clock_source = RTC_CLOCK_SOURCE_LSI;
        }
    }
    else if (rtc_clock_configure_lse() == 0)
    {
        rtc_clock_source = RTC_CLOCK_SOURCE_LSE;
    }
    else if (rtc_clock_configure_lsi() == 0)
    {
        rtc_clock_source = RTC_CLOCK_SOURCE_LSI;
    }

    if (rtc_clock_source == RTC_CLOCK_SOURCE_NONE)
    {
        Error_Handler();
    }

    rtc_clock_configure_handle(rtc_clock_source);

    if (HAL_RTC_Init(&hrtc) != HAL_OK)
    {
        Error_Handler();
    }

    if (HAL_RTC_WaitForSynchro(&hrtc) != HAL_OK)
    {
        Error_Handler();
    }
}

int RTC_CLOCK_SetDateTime(const RTC_ClockDateTime *date_time)
{
    if (rtc_clock_validate(date_time) != 0)
    {
        return -1;
    }

    RTC_TimeTypeDef rtc_time = {0};
    RTC_DateTypeDef rtc_date = {0};

    rtc_time.Hours = date_time->hour;
    rtc_time.Minutes = date_time->minute;
    rtc_time.Seconds = date_time->second;
    rtc_time.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
    rtc_time.StoreOperation = RTC_STOREOPERATION_RESET;

    rtc_date.Year = (uint8_t)(date_time->year - 2000U);
    rtc_date.Month = date_time->month;
    rtc_date.Date = date_time->day;
    rtc_date.WeekDay = rtc_clock_weekday(
        date_time->year,
        date_time->month,
        date_time->day
    );

    if (HAL_RTC_SetTime(&hrtc, &rtc_time, RTC_FORMAT_BIN) != HAL_OK)
    {
        return -1;
    }

    if (HAL_RTC_SetDate(&hrtc, &rtc_date, RTC_FORMAT_BIN) != HAL_OK)
    {
        return -1;
    }

    HAL_RTCEx_BKUPWrite(
        &hrtc,
        RTC_CLOCK_BACKUP_MARKER_REG,
        RTC_CLOCK_BACKUP_MARKER_VALUE
    );

    return 0;
}

int RTC_CLOCK_GetDateTime(RTC_ClockDateTime *date_time)
{
    if (date_time == NULL)
    {
        return -1;
    }

    RTC_TimeTypeDef rtc_time = {0};
    RTC_DateTypeDef rtc_date = {0};

    if (HAL_RTC_GetTime(&hrtc, &rtc_time, RTC_FORMAT_BIN) != HAL_OK)
    {
        return -1;
    }

    if (HAL_RTC_GetDate(&hrtc, &rtc_date, RTC_FORMAT_BIN) != HAL_OK)
    {
        return -1;
    }

    date_time->year = (uint16_t)(2000U + rtc_date.Year);
    date_time->month = rtc_date.Month;
    date_time->day = rtc_date.Date;
    date_time->hour = rtc_time.Hours;
    date_time->minute = rtc_time.Minutes;
    date_time->second = rtc_time.Seconds;

    return 0;
}

int RTC_CLOCK_IsConfigured(void)
{
    return (
        HAL_RTCEx_BKUPRead(&hrtc, RTC_CLOCK_BACKUP_MARKER_REG) ==
        RTC_CLOCK_BACKUP_MARKER_VALUE
    );
}

const char *RTC_CLOCK_SourceName(void)
{
    if (rtc_clock_source == RTC_CLOCK_SOURCE_LSE)
    {
        return "LSE";
    }

    if (rtc_clock_source == RTC_CLOCK_SOURCE_LSI)
    {
        return "LSI";
    }

    return "none";
}

void HAL_RTC_MspInit(RTC_HandleTypeDef *rtc_handle)
{
    if (rtc_handle->Instance == RTC)
    {
        __HAL_RCC_RTC_ENABLE();
    }
}

static int rtc_clock_configure_lse(void)
{
    rtc_clock_reset_backup_domain();

    if (rtc_clock_start_lse() != 0)
    {
        return -1;
    }

    __HAL_RCC_RTC_CONFIG(RCC_RTCCLKSOURCE_LSE);
    return 0;
}

static int rtc_clock_start_lse(void)
{
    __HAL_RCC_LSE_CONFIG(RCC_LSE_ON);

    uint32_t started_at = HAL_GetTick();
    while (__HAL_RCC_GET_FLAG(RCC_FLAG_LSERDY) == RESET)
    {
        if ((HAL_GetTick() - started_at) >= RTC_CLOCK_LSE_TIMEOUT_MS)
        {
            __HAL_RCC_LSE_CONFIG(RCC_LSE_OFF);
            return -1;
        }
    }

    return 0;
}

static int rtc_clock_configure_lsi(void)
{
    RCC_OscInitTypeDef rcc_osc = {0};

    rcc_osc.OscillatorType = RCC_OSCILLATORTYPE_LSI;
    rcc_osc.LSIState = RCC_LSI_ON;

    if (HAL_RCC_OscConfig(&rcc_osc) != HAL_OK)
    {
        return -1;
    }

    __HAL_RCC_RTC_CONFIG(RCC_RTCCLKSOURCE_LSI);
    return 0;
}

static void rtc_clock_reset_backup_domain(void)
{
    __HAL_RCC_BACKUPRESET_FORCE();
    __HAL_RCC_BACKUPRESET_RELEASE();
}

static void rtc_clock_configure_handle(RTC_ClockSource source)
{
    hrtc.Instance = RTC;
    hrtc.Init.HourFormat = RTC_HOURFORMAT_24;
    hrtc.Init.AsynchPrediv = 127U;
    hrtc.Init.SynchPrediv = (source == RTC_CLOCK_SOURCE_LSE) ? 255U : 249U;
    hrtc.Init.OutPut = RTC_OUTPUT_DISABLE;
    hrtc.Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;
    hrtc.Init.OutPutType = RTC_OUTPUT_TYPE_OPENDRAIN;
}

static int rtc_clock_validate(const RTC_ClockDateTime *date_time)
{
    if (date_time == NULL)
    {
        return -1;
    }

    if ((date_time->year < 2000U) || (date_time->year > 2099U))
    {
        return -1;
    }

    if ((date_time->month < 1U) || (date_time->month > 12U))
    {
        return -1;
    }

    if (
        (date_time->day < 1U) ||
        (date_time->day > rtc_clock_days_in_month(date_time->year, date_time->month))
    )
    {
        return -1;
    }

    if (
        (date_time->hour > 23U) ||
        (date_time->minute > 59U) ||
        (date_time->second > 59U)
    )
    {
        return -1;
    }

    return 0;
}

static uint8_t rtc_clock_weekday(uint16_t year, uint8_t month, uint8_t day)
{
    static const uint8_t offsets[] = {0U, 3U, 2U, 5U, 0U, 3U, 5U, 1U, 4U, 6U, 2U, 4U};

    if (month < 3U)
    {
        --year;
    }

    uint16_t weekday = (uint16_t)(
        year +
        (year / 4U) -
        (year / 100U) +
        (year / 400U) +
        offsets[month - 1U] +
        day
    ) % 7U;

    return (weekday == 0U) ? RTC_WEEKDAY_SUNDAY : (uint8_t)weekday;
}

static uint8_t rtc_clock_days_in_month(uint16_t year, uint8_t month)
{
    static const uint8_t days_per_month[] = {
        31U, 28U, 31U, 30U, 31U, 30U,
        31U, 31U, 30U, 31U, 30U, 31U
    };

    if ((month == 2U) && (rtc_clock_is_leap_year(year) != 0))
    {
        return 29U;
    }

    return days_per_month[month - 1U];
}

static int rtc_clock_is_leap_year(uint16_t year)
{
    return (((year % 4U) == 0U) && (((year % 100U) != 0U) || ((year % 400U) == 0U)));
}

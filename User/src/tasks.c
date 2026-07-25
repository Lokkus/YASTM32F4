#include "tasks.h"

#include "main.h"
#include "rtc_clock.h"
#include "usb_device.h"
#include "usb_stdin.h"
#include "usb_stdout.h"

#include "cmsis_os2.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>


#define TASK_SYSTEM_LOGGER_STACK_SIZE       512U
#define TASK_SYSTEM_LOGGER_PRIORITY         osPriorityLow

#define TASK_USB_DEVICE_STACK_SIZE          512U
#define TASK_USB_DEVICE_PRIORITY            osPriorityNormal

#define TASK_SYSTEM_LOGGER_START_DELAY_MS   1500U

#define TASK_USB_COMMAND_STACK_SIZE         768U
#define TASK_USB_COMMAND_PRIORITY           osPriorityLow
#define TASK_USB_COMMAND_START_DELAY_MS     1600U
#define TASK_USB_COMMAND_LINE_SIZE          96U


static void task_system_logger(void *argument);
static void task_usb_device(void *argument);
static void task_usb_command(void *argument);
static void handle_usb_command(char *line);
static int parse_rtc_set_command(const char *line, RTC_ClockDateTime *date_time);
static int parse_datetime(const char *text, RTC_ClockDateTime *date_time);
static int parse_uint_field(const char *text, uint8_t length, uint16_t *value);
static const char *skip_spaces(const char *text);


static osThreadId_t task_system_logger_handle;
static osThreadId_t task_usb_device_handle;
static osThreadId_t task_usb_command_handle;


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

static const osThreadAttr_t task_usb_command_attributes = {
    .name = "task_usb_command",
    .stack_size = TASK_USB_COMMAND_STACK_SIZE,
    .priority = TASK_USB_COMMAND_PRIORITY
};


void Tasks_Init(void)
{
    USB_STDIN_Init();

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

    task_usb_command_handle = osThreadNew(
        task_usb_command,
        NULL,
        &task_usb_command_attributes
    );

    (void)task_usb_device_handle;
    (void)task_system_logger_handle;
    (void)task_usb_command_handle;
}


static void task_system_logger(void *argument)
{
    (void)argument;

    osDelay(TASK_SYSTEM_LOGGER_START_DELAY_MS);

    USB_STDOUT_Init();
    printf(
        "[task_system_logger] ready, rtc source=%s configured=%s\r\n",
        RTC_CLOCK_SourceName(),
        RTC_CLOCK_IsConfigured() ? "yes" : "no"
    );

    for (;;)
    {
        osDelay(1000U);
    }
}

static void task_usb_device(void *argument)
{
    (void)argument;
    MX_USB_DEVICE_Init();
    osThreadExit();
}

static void task_usb_command(void *argument)
{
    (void)argument;

    osDelay(TASK_USB_COMMAND_START_DELAY_MS);
    printf("[task_usb_command] ready: rtc get | rtc set YYYY-MM-DD HH:MM:SS\r\n");

    char line[TASK_USB_COMMAND_LINE_SIZE];
    uint32_t line_len = 0U;
    uint8_t byte;
    bool line_overflow = false;

    for (;;)
    {
        if (USB_STDIN_GetChar(&byte, osWaitForever) == 0)
        {
            continue;
        }

        if ((byte == '\r') || (byte == '\n'))
        {
            if (line_overflow)
            {
                printf(
                    "[usb command] input too long, max=%lu bytes\r\n",
                    (unsigned long)(TASK_USB_COMMAND_LINE_SIZE - 1U)
                );
                line_len = 0U;
                line_overflow = false;
            }
            else if (line_len > 0U)
            {
                line[line_len] = '\0';
                handle_usb_command(line);
                line_len = 0U;
            }
            continue;
        }

        if (line_len < (TASK_USB_COMMAND_LINE_SIZE - 1U))
        {
            line[line_len] = (char)byte;
            ++line_len;
        }
        else
        {
            line_overflow = true;
        }
    }
}

static void handle_usb_command(char *line)
{
    const char *command = skip_spaces(line);

    if (strcmp(command, "rtc get") == 0)
    {
        RTC_ClockDateTime date_time;

        if (RTC_CLOCK_GetDateTime(&date_time) != 0)
        {
            printf("[rtc] read failed\r\n");
            return;
        }

        printf(
            "[rtc] %04u-%02u-%02u %02u:%02u:%02u source=%s configured=%s\r\n",
            (unsigned int)date_time.year,
            (unsigned int)date_time.month,
            (unsigned int)date_time.day,
            (unsigned int)date_time.hour,
            (unsigned int)date_time.minute,
            (unsigned int)date_time.second,
            RTC_CLOCK_SourceName(),
            RTC_CLOCK_IsConfigured() ? "yes" : "no"
        );
        return;
    }

    if (strncmp(command, "rtc set", strlen("rtc set")) == 0)
    {
        RTC_ClockDateTime date_time;

        if (parse_rtc_set_command(command, &date_time) != 0)
        {
            printf("[rtc] usage: rtc set YYYY-MM-DD HH:MM:SS\r\n");
            return;
        }

        if (RTC_CLOCK_SetDateTime(&date_time) != 0)
        {
            printf("[rtc] set failed\r\n");
            return;
        }

        printf(
            "[rtc] set %04u-%02u-%02u %02u:%02u:%02u source=%s\r\n",
            (unsigned int)date_time.year,
            (unsigned int)date_time.month,
            (unsigned int)date_time.day,
            (unsigned int)date_time.hour,
            (unsigned int)date_time.minute,
            (unsigned int)date_time.second,
            RTC_CLOCK_SourceName()
        );
        return;
    }

    printf("[usb command] unknown command: %s\r\n", command);
}

static int parse_rtc_set_command(const char *line, RTC_ClockDateTime *date_time)
{
    const char *payload = line + strlen("rtc set");
    payload = skip_spaces(payload);

    return parse_datetime(payload, date_time);
}

static int parse_datetime(const char *text, RTC_ClockDateTime *date_time)
{
    uint16_t year;
    uint16_t month;
    uint16_t day;
    uint16_t hour;
    uint16_t minute;
    uint16_t second;

    if (
        (parse_uint_field(&text[0], 4U, &year) != 0) ||
        (text[4] != '-') ||
        (parse_uint_field(&text[5], 2U, &month) != 0) ||
        (text[7] != '-') ||
        (parse_uint_field(&text[8], 2U, &day) != 0) ||
        ((text[10] != ' ') && (text[10] != 'T')) ||
        (parse_uint_field(&text[11], 2U, &hour) != 0) ||
        (text[13] != ':') ||
        (parse_uint_field(&text[14], 2U, &minute) != 0) ||
        (text[16] != ':') ||
        (parse_uint_field(&text[17], 2U, &second) != 0)
    )
    {
        return -1;
    }

    if (text[19] != '\0')
    {
        return -1;
    }

    date_time->year = year;
    date_time->month = (uint8_t)month;
    date_time->day = (uint8_t)day;
    date_time->hour = (uint8_t)hour;
    date_time->minute = (uint8_t)minute;
    date_time->second = (uint8_t)second;

    return 0;
}

static int parse_uint_field(const char *text, uint8_t length, uint16_t *value)
{
    uint16_t result = 0U;

    for (uint8_t i = 0U; i < length; ++i)
    {
        if ((text[i] < '0') || (text[i] > '9'))
        {
            return -1;
        }

        result = (uint16_t)((result * 10U) + (uint16_t)(text[i] - '0'));
    }

    *value = result;
    return 0;
}

static const char *skip_spaces(const char *text)
{
    while (*text == ' ')
    {
        ++text;
    }

    return text;
}

#include "crossSystem_tool.h"
#include "stdarg.h"
#include <time.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

#define ENABLE_TO_PRINT

#ifdef HOST
#include <stdio.h>
#endif
#ifdef AIDECK
#include "pmsis.h"
#include "bsp/bsp.h"
#endif
#ifdef CRAZYFLIE
#include "FreeRTOS.h"
#include "debug.h"
#endif

void sleep_ms(int milliseconds) // cross-platform sleep function
{
#ifdef _WIN32
    Sleep(milliseconds);
#else
    struct timespec ts;
    ts.tv_sec = milliseconds / 1000; // 秒部分
    ts.tv_nsec = (milliseconds % 1000) * 1000000; // 纳秒部分
    nanosleep(&ts, NULL);
#endif
}

void printF(const char *format, ...)// cross-platform print function
{
    #ifndef ENABLE_TO_PRINT
        return;
    #endif
    va_list args;
    va_start(args, format);
#ifdef HOST
    // printf(format, args);
    vprintf(format, args);
#endif
#ifdef AIDECK
    cpxPrintToConsole(LOG_TO_CRTP, format, args);
#endif
#ifdef CRAZYFLIE
    DEBUG_PRINT(format, args);
#endif
    va_end(args);
}
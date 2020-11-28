#ifndef __LOG_H
#define __LOG_H
#include <time.h>
#include <stdio.h>
#include "cycle.h"
#include <stdarg.h>

time_t current_time;
struct timeval current_time_tv;
FILE* log_fp;
int enable_debug_log;

typedef enum {
    LOG_DEBUG,
    LOG_RUN_ERROR,
    LOG_RUN_ALERT
} log_level_t;
#define log(level,fmt,args...) __log(level,fmt,__func__,__FILE__,__LINE__,##args)
void __log(log_level_t level, const char* fmt, const char *func, char *file, int line, ...);
void log_init(const char *);



#define LEN_TIME_FARMAT_RFC1123 sizeof("Sun, 06 Nov 1994 08:49:23 GMT")
#define LEN_TIME_FARMAT_LOG     sizeof("1994-11-06 08:49:23")


typedef char time_format_rfc1123[LEN_TIME_FARMAT_RFC1123];
void timer_format_log(time_t* now, time_format_rfc1123* buf);
void time_update();
#endif

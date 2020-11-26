#include "dfs_head.h"
void timer_format_log(time_t* now, time_format_rfc1123* buf) {
    struct tm tm;

    localtime_r(now, &tm);
    snprintf((char*)buf, LEN_TIME_FARMAT_LOG,
             "%d-%02d-%02d %02d:%02d:%02d",
             tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
             tm.tm_hour, tm.tm_min, tm.tm_sec);
}

void time_update() {
    gettimeofday(&current_time_tv, NULL);
    current_time = current_time_tv.tv_sec;
}

void log_init() {
    time_update();
    log_fp = fopen("run.log", "a+");;
    //log_fp = NULL;
    //set_close_on_exec(fileno(log_fp));
    enable_debug_log = 1;
}

void __log(log_level_t level, const char* fmt, const char *func, char *file, int line, ...) {
    if (!enable_debug_log && level == LOG_DEBUG) {
        return;
    }

    static const char* error_type[] = {
        "DEBUG",
        "ERROR",
        "ALERT"
    };

    time_t now = current_time;
    time_format_rfc1123 tbuf;

    timer_format_log((time_t*)&now, &tbuf);

    va_list args;
    char real_format[4096];
    snprintf(real_format, 4096, "%s [%s] %s, %s, %d, %s",
                    (char*)&tbuf, error_type[level], func, file, line, fmt);
    va_start(args, line);

    FILE* fp = log_fp ? log_fp : stderr;
    /*
    fprintf(fp, "%s [%s] ",
            (char*)&tbuf,
            error_type[level]);
    */
    vfprintf(fp, real_format, args);

    va_end(args);
    fflush(fp);
}

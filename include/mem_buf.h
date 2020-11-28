#ifndef __MEM_BUF_H__
#define __MEM_BUF_H__

#define MEM_BUF_INIT_SIZE   (2*1024)
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef int mb_size_t;

typedef struct {
    /* public, read-only */
    char* buf;
    mb_size_t size;                         /* used space, does not count 0-terminator */

    /* private, stay away; use interface function instead */
    mb_size_t max_capacity;         /* when grows: assert(new_capacity <= max_capacity) */
    mb_size_t capacity;                     /* allocated space */
    unsigned stolen: 1;                     /* the buffer has been stolen for use by someone else */
} mem_buf_t;

/* init with defaults */
#ifdef LEAK_CHECK
void internal_mem_buf_def_init(mem_buf_t* mb, const char* file, int line);
#define mem_buf_def_init(mb) internal_mem_buf_def_init(mb, __FILE__, __LINE__)
#else
void mem_buf_def_init(mem_buf_t* mb);
#endif

void mem_buf_swap(mem_buf_t* buf_size_to_zero, mem_buf_t* buf);

/* init with specific sizes */
#ifdef LEAK_CHECK
void internal_mem_buf_init(mem_buf_t* mb, mb_size_t szInit, mb_size_t szMax, const char* file,
                           int line);
#define mem_buf_init(mb, szInit, szMax) \
        internal_mem_buf_init((mb), (szInit), (szMax), __FILE__, __LINE__)
#else
void mem_buf_init(mem_buf_t* mb, mb_size_t szInit, mb_size_t szMax);
#endif

void mem_buf_clean(mem_buf_t* mb);

#ifdef LEAK_CHECK
void internal_mem_buf_reset(mem_buf_t* mb, const char* file, int line);
#define mem_buf_reset(mb) internal_mem_buf_reset((mb),__FILE__,__LINE__)
#else
void mem_buf_reset(mem_buf_t* mb);
#endif

/* unfortunate hack to test if the buffer has been Init()ialized */
int mem_buf_is_null(mem_buf_t* mb);

/* calls memcpy, appends exactly size bytes, extends buffer if needed */
#ifdef LEAK_CHECK
void internal_mem_buf_append(mem_buf_t* mb, const void* buf, int sz, const char* file,
                             int line);
#define mem_buf_append(mb,buf,sz) internal_mem_buf_append((mb),(buf),(sz),__FILE__,__LINE__)
#else
void mem_buf_append(mem_buf_t* mb, const void* buf, int sz);
#endif

//#if STDC_HEADERS
#define PRINTF_FORMAT_ARG2 __attribute__ ((format (printf, 2, 3)))
void mem_buf_printf(mem_buf_t* mb, const char* fmt, ...)
PRINTF_FORMAT_ARG2;
//#else
//void mem_buf_printf ();
//#endif

/* vprintf for other printf()'s to use; calls vsnprintf, extends buf if needed */
#ifdef LEAK_CHECK
void internal_mem_buf_v_printf(mem_buf_t* mb, const char* file, int line, const char* fmt,
                               va_list vargs);
#define mem_buf_v_printf(mb,fmt,vargs) internal_mem_buf_v_printf((mb),(__FILE__),(__LINE__),(fmt),(vargs))
#else
void mem_buf_v_printf(mem_buf_t* mb, const char* fmt, va_list vargs);
#endif

#endif

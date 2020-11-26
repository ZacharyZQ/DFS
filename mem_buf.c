#include "mem_buf.h"
#include <assert.h>
#include <stdio.h>

#define VA_COPY va_copy

#define MEM_BUF_MAX_SIZE    (2*1000*1024*1024)


/* local routines */
#ifdef LEAK_CHECK
static void internal_mem_buf_grow(mem_buf_t* mb, mb_size_t min_cap, const char* file, int line);
#define mem_buf_grow(mb, min_cap) internal_mem_buf_grow((mb), (min_cap), (__FILE__), (__LINE__))
#else
static void mem_buf_grow(mem_buf_t* mb, mb_size_t min_cap);
#endif


/* init with defaults */
#ifdef LEAK_CHECK
void internal_mem_buf_def_init(mem_buf_t* mb, const char* file, int line) {
    internal_mem_buf_init(mb, MEM_BUF_INIT_SIZE, MEM_BUF_MAX_SIZE, file, line);
}
#else
void mem_buf_def_init(mem_buf_t* mb) {
    mem_buf_init(mb, MEM_BUF_INIT_SIZE, MEM_BUF_MAX_SIZE);
}
#endif

void mem_buf_swap(mem_buf_t* buf_size_to_zero, mem_buf_t* buf)
{
    mem_buf_t tmp = *buf_size_to_zero;
    *buf_size_to_zero = *buf;
    *buf = tmp;
    
    // set read buf size to 0 to read new data from orig
    buf_size_to_zero->size = 0;
}

/* init with specific sizes */
#ifdef LEAK_CHECK
void internal_mem_buf_init(mem_buf_t* mb, mb_size_t szInit, mb_size_t szMax, const char* file,
                           int line)
#else
void mem_buf_init(mem_buf_t* mb, mb_size_t szInit, mb_size_t szMax)
#endif
{
    assert(mb);
    assert(szInit > 0 && szMax > 0);

    mb->buf = NULL;
    mb->size = 0;
    mb->max_capacity = szMax;
    mb->capacity = 0;
    mb->stolen = 0;
#ifdef LEAK_CHECK
    internal_mem_buf_grow(mb, szInit, file, line);
#else
    mem_buf_grow(mb, szInit);
#endif
}

/*
 * cleans the mb; last function to call if you do not give .buf away with
 * memBufFreeFunc
 */
void mem_buf_clean(mem_buf_t* mb) {
    assert(mb);
    assert(mb->buf);
    assert(!mb->stolen);        /* not frozen */

    free(mb->buf);
    mb->buf = NULL;
    mb->size = mb->capacity = mb->max_capacity = 0;
}

/* cleans the buffer without changing its capacity
 * if called with a Null buffer, calls memBufDefInit() */
#ifdef LEAK_CHECK
void internal_mem_buf_reset(mem_buf_t* mb, const char* file, int line)
#else
void mem_buf_reset(mem_buf_t* mb)
#endif
{
    assert(mb);

    if (mem_buf_is_null(mb)) {
#ifdef LEAK_CHECK
        internal_mem_buf_def_init(mb, file, line);
#else
        mem_buf_def_init(mb);
#endif
    } else {
        assert(!mb->stolen);    /* not frozen */
        /* reset */
        mb->buf = NULL;
        mb->size = 0;
#ifdef LEAK_CHECK
        internal_mem_buf_def_init(mb, file, line);
#else
        mem_buf_def_init(mb);
#endif
    }
}

/* unfortunate hack to test if the buffer has been Init()ialized */
int mem_buf_is_null(mem_buf_t* mb) {
    assert(mb);

    if (!mb->buf && !mb->max_capacity && !mb->capacity && !mb->size) {
        return 1;    /* is null (not initialized) */
    }

    assert(mb->buf && mb->max_capacity && mb->capacity);    /* paranoid */
    return 0;
}


/* calls memcpy, appends exactly size bytes, extends buffer if needed */
#ifdef LEAK_CHECK
void internal_mem_buf_append(mem_buf_t* mb, const void* buf, int sz, const char* file, int line)
#else
void mem_buf_append(mem_buf_t* mb, const void* buf, int sz)
#endif
{
    assert(mb && buf && sz >= 0);
    assert(mb->buf);
    assert(!mb->stolen);        /* not frozen */

    if (sz > 0) {
        if (mb->size + sz + 1 > mb->capacity) {
#ifdef LEAK_CHECK
            internal_mem_buf_grow(mb, mb->size + sz + 1, file, line);
#else
            mem_buf_grow(mb, mb->size + sz + 1);
#endif
        }

        assert(mb->size + sz <= mb->capacity);  /* paranoid */
        memcpy(mb->buf + mb->size, buf, sz);
        mb->size += sz;
        mb->buf[mb->size] =
            '\0';   /* \0 terminate in case we are used as a string. Not counted in the size */
    }
}

/* calls memBufVPrintf */
//#if STDC_HEADERS
void mem_buf_printf(mem_buf_t* mb, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    /*
    #else
    void mem_buf_printf (va_alist)
    va_dcl
    {
    va_list args;
    mem_buf_t*mb = NULL;
    const char *fmt = NULL;
    mb_size_t sz = 0;

    va_start (args);
    mb = va_arg (args, mem_buf_t*);
    fmt = va_arg (args, char *);
    #endif
    */
    mem_buf_v_printf(mb, fmt, args);
    va_end(args);
}


/* vprintf for other printf()'s to use; calls vsnprintf, extends buf if needed */
#ifdef LEAK_CHECK
void internal_mem_buf_v_printf(mem_buf_t* mb, const char* file, int line, const char* fmt,
                               va_list vargs)
#else
void mem_buf_v_printf(mem_buf_t* mb, const char* fmt, va_list vargs)
#endif
{
#if defined VA_COPY
    va_list ap;
#endif
    int sz = 0;

    assert(mb && fmt);
    assert(mb->buf);
    assert(!mb->stolen);        /* not frozen */

    /* assert in Grow should quit first, but we do not want to have a scary infinite loop */
    while (mb->capacity <= mb->max_capacity) {
        mb_size_t free_space = mb->capacity - mb->size;

        /* put as much as we can */

#if defined VA_COPY
        VA_COPY(ap, vargs);
        sz = vsnprintf(mb->buf + mb->size, free_space, fmt, ap);
        va_end(ap);
#else
        sz = vsnprintf(mb->buf + mb->size, free_space, fmt, vargs);
#endif

        /* check for possible overflow */
        /* snprintf on Linuz returns -1 on overflows */
        /* snprintf on FreeBSD returns at least free_space on overflows */
        if (sz < 0 || sz >= free_space) {
#ifdef LEAK_CHECK
            internal_mem_buf_grow(mb, mb->capacity + 1, file, line);
#else
            mem_buf_grow(mb, mb->capacity + 1);
#endif
        } else {
            break;
        }
    }

    mb->size += sz;

    /* on Linux and FreeBSD, '\0' is not counted in return value */
    /* on XXX it might be counted */
    /* check that '\0' is appended and not counted */
    if (!mb->size || mb->buf[mb->size - 1]) {
        assert(!mb->buf[mb->size]);
    } else {
        mb->size--;
    }
}

/* grows (doubles) internal buffer to satisfy required minimal capacity */
#ifdef LEAK_CHECK
static void internal_mem_buf_grow(mem_buf_t* mb, mb_size_t min_cap, const char* file, int line)
#else
static void mem_buf_grow(mem_buf_t* mb, mb_size_t min_cap)
#endif
{
    size_t new_cap = 0;

    assert(mb);
    assert(!mb->stolen);
    assert(mb->capacity < min_cap);

    /* determine next capacity */
    if (min_cap > 64 * 1024) {
        new_cap = 64 * 1024;

        while (new_cap < (size_t) min_cap) {
            new_cap += 64 * 1024;    /* increase in reasonable steps */
        }
    } else {
        new_cap = (size_t) min_cap;
    }

    /* last chance to fit before we assert(!overflow) */
    if (new_cap > (size_t) mb->max_capacity) {
        new_cap = (size_t) mb->max_capacity;
    }

    assert(new_cap <= (size_t) mb->max_capacity);   /* no overflow */
    assert(new_cap > (size_t) mb->capacity);    /* progress */

#ifdef LEAK_CHECK
    mb->buf = (char*)internal_xrealloc(mb->buf, new_cap, file, line);
#else
    mb->buf = (char*)realloc(mb->buf, new_cap);
#endif

    /* done */
    mb->capacity = (mb_size_t) new_cap;
}

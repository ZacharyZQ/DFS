#ifndef __MD5_H__
#define __MD5_H__
#include <stdint.h>
typedef struct _md5_context {
    uint32_t buf[4];
    uint32_t bytes[2];
    uint32_t in[16];
} md5_context;

void md5_init(md5_context* context);
void md5_update(md5_context* context, const void* buf, unsigned len);
void md5_final(uint8_t digest[16], md5_context* context);
void md5_transform(uint32_t buf[4], uint32_t const in[16]);
void md5_expand(unsigned char* in, char* out);

#define MD5_DIGEST_LENGTH         16

#endif

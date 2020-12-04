#ifndef __HASH_H__
#define __HASH_H__
#include <stdint.h>
typedef void HASHFREE(void*);
typedef int HASHCMP(const void*, const void*);
typedef unsigned int HASHHASH(const void*, unsigned int);
typedef unsigned int HASHNHASH(const void* key, unsigned int size, size_t len);

typedef struct _hash_link {
    char* key;
    struct _hash_link* next;
} hash_link;

typedef struct _hash_table {
    //a store_hash_link array
    hash_link** buckets;
    HASHCMP* cmp;
    //hash function, key->index
    HASHHASH* hash;
    //num of buckets
    unsigned int size;
    //for traverse, not used now
#if 0
    unsigned int current_slot;
    hash_link* next;
#endif
    int count;
} hash_table;

unsigned int hash_n_string(const void* data, unsigned int size, size_t len);

extern hash_table* hash_create(HASHCMP*, int, HASHHASH*);
extern void hash_join(hash_table*, hash_link*);
extern void hash_remove_link(hash_table*, hash_link*);
extern void* hash_lookup(hash_table*, const void*);
extern void hash_free_memory(hash_table*);
extern HASHHASH hash_string;
extern HASHHASH hash_md5key;
int int_key_cmp(const void* str1, const void* str2);
int md5_cmp(const void* str1, const void* str2);
#endif

#include <stdlib.h>
#include <assert.h>
#include "hash.h"
#include "dfs_head.h"

hash_table_t* hash_create(HASHCMP* cmp, int size, HASHHASH* hash_func) {
    hash_table_t* hid = (hash_table_t*)calloc(1, sizeof(hash_table_t));

    hid->size = (unsigned int) size;
    /* allocate and null the buckets */
    hid->buckets = (hash_link**)calloc(hid->size, sizeof(hash_link*));
    unsigned int i;

    for (i = 0; i < hid->size; i ++) {
        hid->buckets[i] = NULL;
    }

    hid->cmp = cmp;
    hid->hash = hash_func;
    return hid;
}
void hash_join(hash_table_t* hid, hash_link* hl) {
    int i;
    i = hid->hash(hl->key, hid->size);
    hl->next = hid->buckets[i];
    hid->buckets[i] = hl;
    hid->count++;
}

//only remove from hash table, not freeing item
void hash_remove_link(hash_table_t* hid, hash_link* hl) {
    assert(hl != NULL);
    assert(hl->key);
    int i;
    i = hid->hash(hl->key, hid->size);

    hash_link** P;

    for (P = &hid->buckets[i]; *P; P = &(*P)->next) {
        if (*P != hl) {
            continue;
        }

        *P = hl->next;
        hid->count--;
        return;
    }

    assert(hl->key);
    assert(0);
}

void* hash_lookup(hash_table_t* hid, const void* k) {
    assert(k != NULL);
    unsigned int b = hid->hash(k, hid->size);
    int i = 0;
    hash_link* walker = NULL;
    hash_link* prev = NULL;

    for (walker = hid->buckets[b], prev = hid->buckets[b]; 
            walker != NULL; walker = walker->next, i++) 
    {
        /* strcmp of NULL is a SEGV */
        if (NULL == walker->key) {
            return NULL;
        }

        if ((hid->cmp)(k, walker->key) == 0) 
        {
            if(i > 0)
            {
                prev->next = walker->next;
                walker->next = hid->buckets[b];
                hid->buckets[b] = walker;
            }
            return (walker);
        }

        assert(walker != walker->next);
        prev = walker;
    }

    return NULL;
}

void hash_free_memory(hash_table_t* hid) {
    assert(hid != NULL);

    if (hid->buckets) {
        free(hid->buckets);
    }

    free(hid);
}

unsigned int hash_string(const void* data, unsigned int size) {
    const char* s = (const char*)data;
    unsigned int n = 0;
    unsigned int j = 0;
    unsigned int i = 0;

    while (*s) {
        j++;
        n ^= 271 * (unsigned) * s++;
    }

    i = n ^ (j * 271);
    return i % size;
}


unsigned int hash_n_string(const void* data, unsigned int size, size_t len) {
    const char* s = (const char*)data;
    unsigned int n = 0;
    unsigned int j = 0;
    unsigned int i = 0;

    while (len > 0) {
        j++;
        n ^= 271 * (unsigned) * s++;
        len--;
    }

    i = n ^ (j * 271);
    return i % size;
}


unsigned int hash_md5key(const void* data, unsigned int size) {
    const uint32_t* p = (const uint32_t*)data;
    return *p % size;
}

int int_key_cmp(const void* str1, const void* str2) {
    return memcmp(str1, str2, sizeof(int));
}

int md5_cmp(const void* str1, const void* str2) {
    return memcmp(str1, str2, 16);
}

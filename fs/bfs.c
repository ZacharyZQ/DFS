#include "dfs_head.h"
#include "log2.h"

extern disk_alloc_ops default_disk_alloc;
bfs_t *bfs;
int bfs_init() {
    char *disk_name = "./disk/disk";
    struct stat path_stat;
    bfs = calloc(1, sizeof(bfs_t));
    bfs->bmt = block_manager_init();
    bfs->disk_alloc = &default_disk_alloc;
    bfs->disk_fd = open(disk_name, O_RDWR);
    unsigned sector_num;
    if (bfs->disk_fd < 0) {
        log(LOG_RUN_ERROR, "open disk /%s/ error, %d\n", disk_name, errno);
        assert(0);
    }
    if (fstat(bfs->disk_fd, &path_stat) < 0) {
        log(LOG_RUN_ERROR, "fstat error /%s/ %d\n", disk_name, errno);
        assert(0);
    }
    if (S_ISREG(path_stat.st_mode)) {
        bfs->disk_size = path_stat.st_size;
    } else if (S_ISBLK(path_stat.st_mode)) {
        if (ioctl(bfs->disk_fd, BLKGETSIZE, &sector_num) < 0) { 
            log(LOG_RUN_ERROR, "ioct \'%s\' errno %d\n", disk_name, errno);
            assert(0);
        }
        /* in linux/fs.h, BLKGETSIZE return device size /512 */
        
        bfs->disk_size = sector_num << 9;
    } else {
        log(LOG_RUN_ERROR, "disk is not reg or blk\n");
        assert(0);
    }
    if (bfs->disk_alloc->init_disk_alloc(bfs->disk_size / PAGE_SIZE) < 0) {
        log(LOG_RUN_ERROR, "init_disk_alloc failed\n");
        assert(0);
    }
    log(LOG_RUN_ERROR, "bfs init succ, disk size %lu\n", bfs->disk_size);
    return 0;
}

block_manager_t *block_manager_init() {
    block_manager_t *bmt = calloc(1, sizeof(block_manager_t));
    bmt->block_hash_table = hash_create(md5_cmp, 9999991, hash_md5key);
    bmt->lru_size = 1000;
    bmt->lru_count = 0;
    INIT_LIST_HEAD(&bmt->LRU_head);
    return bmt;
}

block_location_t *init_block(unsigned char key[16], uint64_t offset,
        uint32_t length, char *buf)
{
    block_location_t *bl = calloc(1, sizeof(block_location_t));
    INIT_LIST_HEAD(&bl->lru);
    memcpy(bl->key, key, 16);
    bl->hash.key = (char *)bl->key;
    bl->offset = offset;
    bl->length = length;
    bl->buf = buf;
    bl->flag = B_MEM | B_DISK;
    bfs_write(buf, length, offset);
    hash_join(bfs->bmt->block_hash_table, &bl->hash);
    update_lru(bfs->bmt, bl);
    return bl;
}

block_location_t *get_block(unsigned char key[16]) {
    block_location_t *bl = (block_location_t *)hash_lookup(bfs->bmt->block_hash_table, key);
    assert(bl);
    assert(bl->flag & B_DISK);
    if (!(bl->flag & B_MEM)) {
        assert(!bl->buf);
        bl->buf = calloc(1, bl->length);
        bfs_read(bl->buf, bl->length, bl->offset);
        bl->flag |= B_MEM;
    }
    update_lru(bfs->bmt, bl);
    return bl;
}

void update_lru(block_manager_t *bmt, block_location_t *bl) {
    assert(bl->flag & B_MEM);
#if 0
    if (!(bl->flag & B_MEM)) {
        assert(!bl->buf);
        bl->buf = calloc(1, bl->length);
        bfs_read(bl->buf, bl->length, bl->offset);
        bl->flag |= B_MEM;
    }
#endif
    if (!list_empty(&bl->lru)) {
        list_del(&bl->lru);
        bmt->lru_count --;
    }
    list_add(&bl->lru, &bmt->LRU_head);
    bmt->lru_count ++;
    if (bmt->lru_count > bmt->lru_size) {
        struct list_head *lru_tail = list_last(&bmt->LRU_head);
        block_location_t *last_bl = list_entry(lru_tail, block_location_t, lru);
        assert((last_bl->flag & B_MEM) && last_bl->buf);
        list_del(lru_tail);
        bmt->lru_count --;
        if (!(last_bl->flag & B_DISK)) {
            bfs_write(last_bl->buf, last_bl->length, last_bl->offset);
            last_bl->flag |= B_DISK;
        }
        last_bl->flag &= ~B_MEM;
        free(last_bl->buf);
        last_bl->buf = NULL;
    }
}

void bfs_write(char *buf, uint32_t length, uint64_t offset) {
    int cursor = 0;
    while (cursor < length) {
        int write_size = pwrite(bfs->disk_fd, buf + cursor, length - cursor, offset + cursor);
        if (write_size <= 0) {
            int my_errno = errno;
            if (errno_ignorable(errno)) {
                continue;
            }
            log(LOG_RUN_ERROR, "pwrite error. errno %d, offset %lu, length %u\n",
                    my_errno, offset, length);
            assert(0);
        }
        cursor += write_size;
    }
}

void bfs_read(char *buf, uint32_t length, uint64_t offset) {
    int cursor = 0;
    while (cursor < length) {
        int read_size = pread(bfs->disk_fd, buf + cursor, length - cursor, offset + cursor);
        if (read_size <= 0) {
            int my_errno = errno;
            if (errno_ignorable(errno)) {
                continue;
            }
            log(LOG_RUN_ERROR, "pwrite error. errno %d, offset %lu, length %u\n",
                    my_errno, offset, length);
            assert(0);
        }
        cursor += read_size;
    }
}

int default_init_disk_alloc(uint64_t page_num) {
    default_disk_alloc_data_t *dad = calloc(1, sizeof(default_disk_alloc_data_t));
    bfs->disk_alloc->private_data = dad;
    dad->total_page_num = page_num;
    dad->free_page_num = page_num;
    dad->max_alloc_page = _BLOCK_SIZE / PAGE_SIZE;
    dad->free_space_size = order_base_2(_BLOCK_SIZE / PAGE_SIZE) + 1;
    dad->free_space = calloc(dad->free_space_size, sizeof(rbtree_t));
    int i;
    for (i = 0; i < dad->free_space_size; i ++) {
        rbtree_init(&dad->free_space[i], rbtree_insert_value);
    }
    //free_space[i] contain 1 << i
    uint64_t offset = 0;
    uint64_t unsplit_page;
    int index;
    while (offset < page_num) {
        unsplit_page = page_num - offset;
        if (unsplit_page > dad->max_alloc_page) {
            unsplit_page = dad->max_alloc_page;
        }
        unsplit_page = rounddown_pow_of_two(unsplit_page);
        index = order_base_2(unsplit_page);
        rbtree_node_t *node = calloc(1, sizeof(rbtree_node_t));
        node->key = offset;
        rbtree_insert(&dad->free_space[index], node);
        offset += unsplit_page;
    }
    return 0;
}

int64_t default_page_alloc(uint64_t size) {
    default_disk_alloc_data_t *dad = bfs->disk_alloc->private_data;
    if (size <= 0 || size > dad->max_alloc_page) {
        log(LOG_RUN_ERROR, "support alloc max page is %d, but need alloc %lu\n",
                dad->max_alloc_page, size);
        return -1;
    }
    uint64_t find_size = roundup_pow_of_two(size);
    int index = order_base_2(find_size);
    rbtree_node_t *node = NULL;
    for (; index < dad->free_space_size; index ++) {
        if (!rbtree_empty(&dad->free_space[index])) {
            node = rbtree_min(dad->free_space[index].root, dad->free_space[index].sentinel);
            break;
        }
    }
    if (!node) {
        log(LOG_RUN_ERROR, "alloc %lu page error, no free space\n", size);
        return -1;
    }
    int old_index = index;
    uint64_t ret = node->key;
    uint64_t offset = node->key + size;
    uint64_t right_offset = (1 << index) + node->key;
    uint64_t unsplit_page;
    //right_offset is aligned
    while (offset < right_offset) {
        unsplit_page = right_offset - offset;
        if (unsplit_page > dad->max_alloc_page) {
            unsplit_page = dad->max_alloc_page;
        }
        unsplit_page = rounddown_pow_of_two(unsplit_page);
        index = order_base_2(unsplit_page);
        rbtree_node_t *new_node = calloc(1, sizeof(rbtree_node_t));
        new_node->key = right_offset - unsplit_page;
        rbtree_insert(&dad->free_space[index], new_node);
        right_offset -= unsplit_page;
    }
    rbtree_delete(&dad->free_space[old_index], node);
    free(node);
    dad->free_page_num -= size;
    return ret;
}


disk_alloc_ops default_disk_alloc = {
    .init_disk_alloc = default_init_disk_alloc,
    .page_alloc = default_page_alloc,
};

//not implement
uint64_t get_page_num() {
    default_disk_alloc_data_t *dad = bfs->disk_alloc->private_data;
    return dad->free_page_num;
}

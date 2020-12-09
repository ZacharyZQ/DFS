#ifndef __DFS_HEAD_H
#define __DFS_HEAD_H
#include <stddef.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <time.h>
#include <errno.h>
#include <unistd.h>
#include <linux/fs.h>
#include <arpa/inet.h>
#include <assert.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <ctype.h>
#include <pthread.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include "cycle.h"
#include "dir_tree.h"
#include "epoll.h"
#include "hash.h"
#include "list.h"
#include "log.h"
#include "md5.h"
#include "mem_buf.h"
#include "rbtree.h"
#include "ms_client.h"
#include "ms_upstream.h"
#include "name_node.h"
#include "fs.h"
#include "master.h"
#include "io_thread.h"
#include "bfs.h"

#define _GNU_SOURCE
#define BACK_UP_COUNT 2
#endif

#pragma once
#include <stdio.h>
#include <pthread.h>
#include <time.h>
#include <stdint.h>
#include "utils.h"

#define LOG0(fmt, ...) do { \
    struct timespec _ts; clock_gettime(CLOCK_REALTIME, &_ts); \
    printf("[%ld.%03ld][tid:%lu] " fmt "\n", \
        _ts.tv_sec, _ts.tv_nsec/1000000, (unsigned long)pthread_self(), ##__VA_ARGS__); \
} while (0)

#define LOG1(fmt, ...) do {                                     \
    struct timespec _ts;                                       \
    char _tname[16] = {0};                                     \
    clock_gettime(CLOCK_REALTIME, &_ts);                        \
    pthread_getname_np(pthread_self(), _tname, sizeof(_tname));\
    printf("[%ld.%03ld][%s][tid:%lu] " fmt "\n",                \
        _ts.tv_sec, _ts.tv_nsec / 1000000,                     \
        _tname[0] ? _tname : "noname",                          \
        (unsigned long)pthread_self(),                          \
        ##__VA_ARGS__);                                         \
} while (0)

#define LOG(fmt, ...) do {                                     \
    struct timespec _ts;                                       \
    char _tname[16] = {0};                                     \
    clock_gettime(CLOCK_REALTIME, &_ts);                        \
    pthread_getname_np(pthread_self(), _tname, sizeof(_tname));\
    printf("[%ld.%03ld][%s]" fmt "\n",                \
        _ts.tv_sec, _ts.tv_nsec / 1000000,                     \
        _tname[0] ? _tname : "noname",                          \
        ##__VA_ARGS__);                                         \
} while (0)


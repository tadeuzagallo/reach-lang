#pragma once

#include <cstdio>

#define ASSERT(expr, msg...) \
    do { \
        if (!(expr)) { \
            fprintf(stderr, msg); \
            fputc('\n', stderr); \
            *(int*)0xbbadbeef = 0; \
        } \
    } while (false);

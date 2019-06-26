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


#define ASSERT_NOT_REACHED() \
    ASSERT(false, "Should not be reached")

#define UNUSED(variable) \
    ((void)variable)

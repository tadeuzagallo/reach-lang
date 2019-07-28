#pragma once

#include <iostream>

#define STRINGIFY_(x) #x
#define STRINGIFY(x) STRINGIFY_(x)

#define LOG_CHANNEL_ENABLED(__channel) \
    std::getenv(STRINGIFY(LOG_##__channel))

#define LOG(__channel, ...) \
    do { \
        if (LOG_CHANNEL_ENABLED(__channel)) \
            std::cerr << "[" #__channel "] " << __VA_ARGS__ << std::endl; \
    } while(false)

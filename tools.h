#ifndef TOOLS_H
#define TOOLS_H
#include <unistd.h>
#include <sys/time.h>

#define msleep(x) usleep((x)*1000)

static inline unsigned long long int current_ms()
{
    unsigned long long int res = 0;
    struct timeval tv;

    if (gettimeofday(&tv, NULL)) {
        return 0;
    }
    else {
        res = tv.tv_sec * 1000;
        res += tv.tv_usec / 1000;
    }
    return res;
}

#endif // TOOLS_H


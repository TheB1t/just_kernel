#include <klibc/math.h>

int32_t abs(int32_t in) {
    if (in < 0) return -in;
    return in;
}

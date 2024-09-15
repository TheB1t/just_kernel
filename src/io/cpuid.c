#include <io/cpuid.h>

extern uint64_t _cpuid(CPUID_Request_t request);

uint64_t cpuid(CPUID_Request_t request) {
    return _cpuid(request);
}

uint32_t _cpuid_string(CPUID_Request_t request, uint32_t* ebx, uint32_t* ecx, uint32_t* edx) {
    uint32_t ret;

    asm volatile("cpuid"
                         :"=a"(ret), "=b"(*ebx), "=c"(*ecx), "=d"(*edx)
                         :"a"(request)
                );
    return ret;
}

void _to_string(uint32_t part0, uint32_t part1, uint32_t part2, uint32_t part3, CPUID_String_t str) {
    uint32_t parts[4] = { part0, part1, part2, part3 };

    int offset = 0;
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            str[offset++] = (char)((parts[i] >> (j * 8)) & 0xFF);
        }
    }

    str[16] = '\0';
}

uint32_t cpuid_vendor(CPUID_String_t str) {
    uint32_t ebx, ecx, edx;

    uint32_t ret = _cpuid_string(CPUID_GETVENDORSTRING, &ebx, &ecx, &edx);

    _to_string(ebx, edx, ecx, 0, str);

    return ret;
}

void cpuid_string(CPUID_Request_t request, CPUID_String_t str) {
    uint32_t eax, ebx, ecx, edx;

    eax = _cpuid_string(request, &ebx, &ecx, &edx);

    _to_string(eax, ebx, ecx, edx, str);
}
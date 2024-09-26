#include <klibc/string.h>

uint32_t strlen(char str[]) {
    uint32_t len = 0;
    while (str[len++] != '\0');
    return len - 1;
}

void strcpy(char *src, char *dst) {
    uint32_t len = strlen(src);
    for (uint32_t i = 0; i < len; i++)
        *dst++ = *src++;
    *dst = '\0';
}

void strncpy(char *src, char *dst, uint32_t max) {
    uint32_t len = strlen(src) + 1;
    len = len > max ? max : len;

    for (uint32_t i = 0; i < len; i++)
        *dst++ = *src++;

    *dst = '\0';
}

int32_t strcmp(char *s1, char *s2) {
    if (strlen(s1) != strlen(s2)) return 1;
    while (*s1 != '\0' && *s2 != '\0') {
        if (*s1 != *s2) return 1;
        s1++;
        s2++;
    }
    return 0;
}

int32_t strncmp(char *s1, char *s2, uint32_t len) {
    for (uint32_t i = 0; i < len; i++) {
        if (s1[i] != s2[i]) return 1;
    }
    return 0;
}

char* strchr(char *str, int32_t c) {
    while (*str) {
        if (*str == (char)c)
            return (char*)str;
        str++;
    }

    return (*str == (char)c) ? (char *)str : NULL;
}

char* strtok(char* str, char* delim) {
    static char* next_token = NULL;

    if (str != NULL)
        next_token = str;

    if (next_token == NULL)
        return NULL;

    char *token_start = next_token;
    while (*token_start && strchr(delim, *token_start))
        token_start++;

    if (*token_start == '\0') {
        next_token = NULL;
        return NULL;
    }

    char *token_end = token_start;
    while (*token_end && !strchr(delim, *token_end))
        token_end++;

    if (*token_end) {
        *token_end = '\0';
        next_token = token_end + 1;
    } else {
        next_token = NULL;
    }

    return token_start;
}

void reverse(char str[]) {
    uint32_t start_index = 0;
    uint32_t end_index = 0;
    if (strlen(str) > 0) {
        end_index = strlen(str) - 1;
    }
    char temp_buffer = 0;

    // Two indexes, one starts at 0, the other starts at strlen - 1
    // On each iteration, store the char at start_index in temp_buffer,
    // Store the char at end_index in the space at start_index,
    // And then put the char in temp buffer in the space at end_index
    // And finally decrement end_index and increment start_index
    while (start_index < end_index) {
        temp_buffer = str[start_index];
        str[start_index] = str[end_index];
        str[end_index] = temp_buffer;
        start_index++;
        end_index--;
    }
}

void _vtoa(char* result, uint32_t base, int32_t value) {
    if (base < 2 || base > 36) {
        *result = '\0';
    }

    char *ptr = result, *ptr1 = result, tmp_char;
    int32_t tmp_value;

    do {
        tmp_value = value;
        value /= base;
        *ptr++ = "zyxwvutsrqponmlkjihgfedcba9876543210123456789abcdefghijklmnopqrstuvwxyz"[35 + (tmp_value - value * base)];
    } while (value);

    if (tmp_value < 0) *ptr++ = '-';
    *ptr-- = '\0';
    while (ptr1 < ptr) {
        tmp_char = *ptr;
        *ptr-- = *ptr1;
        *ptr1++ = tmp_char;
    }
}

void itoa(int32_t n, char str[]) {
    _vtoa(str, 10, n);
}

void utoa(uint32_t n, char str[]) {
    uint32_t i;
    i = 0;
    if (n == 0) {
        str[i++] = '0';
    }
    while (n > 0) {
        str[i++] = (n % 10) + '0'; // Calculate the current lowest value digit and convert it
        n /= 10;
    }

    str[i] = '\0';

    reverse(str);
}

uint32_t atou(char str[]) {
    uint32_t result = 0;
    for(; *str; ++str) {
        result *= 10;
        result += *str - '0';
    }

    return result;
}

void htoa(uint32_t in, char str[]) {
    _vtoa(str, 16, in);
}



void strcat(char *dst, char *src) {
    char *end = dst + strlen(dst);

    while (*src != '\0') *end++ = *src++;
    *end = '\0';
}

void memcpy(uint8_t *src, uint8_t *dst, uint32_t count) {
    for (uint32_t i = 0; i<count; i++)
        *dst++ = *src++;
}

void memcpy32(uint32_t *src, uint32_t *dst, uint32_t count) {
    for (uint32_t i = 0; i<count; i++)
        *dst++ = *src++;
}

void memset(uint8_t *dst, uint8_t data, uint32_t count) {
    for (uint32_t i = 0; i<count; i++)
        *dst++ = data;
}

void memset32(uint32_t *dst, uint32_t data, uint32_t count) {
    for (uint32_t i = 0; i<count; i++)
        *dst++ = data;
}
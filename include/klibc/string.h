#pragma once

#include <klibc/stdint.h>
#include <klibc/math.h>

/* String manipulation functions */
void        reverse(char s[]);
void        strcpy(char *src, char *dst);
void        strncpy(char *src, char *dst, int max);
int         strcmp(char *s1, char *s2);
int         strncmp(char *s1, char *s2, int len);
void        strcat(char *dst, char *src);
uint32_t    strlen(char str[]);

/* Integer functions */
void        utoa(uint32_t n, char str[]);
uint32_t    atou(char str[]);
void        itoa(int32_t n, char str[]);
void        htoa(uint32_t in, char str[]);

/* Memory functions */
void        memcpy(uint8_t *src, uint8_t *dst, uint32_t count);
void        memcpy32(uint32_t *src, uint32_t *dst, uint32_t count);
void        memset(uint8_t *dst, uint8_t data, uint32_t count);
void        memset32(uint32_t *dst, uint32_t data, uint32_t count);
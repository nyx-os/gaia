#include <stdc-shim/string.h>

void *memcpy(void *dest, const void *src, size_t n)
{
    char *d = dest;
    const char *s = src;
    while (n--)
    {
        *d++ = *s++;
    }
    return dest;
}

void *memset(void *dest, int c, size_t n)
{
    char *d = dest;
    while (n--)
    {
        *d++ = c;
    }
    return dest;
}

int memcmp(const void *s1, const void *s2, size_t n)
{
    for (size_t i = 0; i < n; i++)
    {
        if (((const char *)s1)[i] != ((const char *)s2)[i])
        {
            return ((const char *)s1)[i] - ((const char *)s2)[i];
        }
    }
    return 0;
}

size_t strlen(const char *s)
{
    size_t i = 0;
    while (s[i])
    {
        i++;
    }
    return i;
}

char *strrev(char *s)
{
    char *p1, *p2;

    if (!s || !*s)
        return s;

    for (p1 = s, p2 = s + strlen(s) - 1; p2 > p1; ++p1, --p2)
    {
        char tmp = *p1;
        *p1 = *p2;
        *p2 = tmp;
    }

    return s;
}

char *strrchr(const char *s, int c)
{
    char *p = NULL;
    while (*s)
    {
        if (*s == c)
        {
            p = (char *)s;
        }
        s++;
    }
    return p;
}

int strncmp(const char *s1, const char *s2, size_t n)
{
    return memcmp(s1, s2, n);
}

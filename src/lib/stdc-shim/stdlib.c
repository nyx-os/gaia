#include <stdc-shim/stdlib.h>
#include <stdc-shim/string.h>

const char *itoa(int64_t value, char *str, int base)
{
    if (base < 2 || base > 36)
    {
        return NULL;
    }

    if (value == 0)
    {
        str[0] = '0';
        str[1] = 0;
        return str;
    }

    int64_t tmp = value;

    if (tmp < 0)
        tmp = -tmp;

    int i = 0;
    while (tmp > 0)
    {
        str[i++] = "0123456789abcdefghijklmnopqrstuvwxyz"[tmp % base];
        tmp /= base;
    }

    if (value < 0)
    {
        str[i++] = '-';
    }

    str[i] = '\0';

    strrev(str);

    return str;
}

const char *utoa(uint64_t value, char *str, int base)
{
    if (base < 2 || base > 36)
    {
        return NULL;
    }

    if (value == 0)
    {
        str[0] = '0';
        str[1] = 0;
        return str;
    }

    uint64_t tmp = value;

    int i = 0;
    while (tmp > 0)
    {
        str[i++] = "0123456789abcdefghijklmnopqrstuvwxyz"[tmp % base];
        tmp /= base;
    }

    str[i] = '\0';
    strrev(str);

    return str;
}

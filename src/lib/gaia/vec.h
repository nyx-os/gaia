// Adaptation of RXI's vec.h, licensed under MIT.

#ifndef LIB_GAIA_VEC_H
#define LIB_GAIA_VEC_H
#include <gaia/base.h>
#include <stdc-shim/string.h>

void vec_expand(void **data, size_t *length, size_t *capacity, int memsz);

#define Vec(T)           \
    struct               \
    {                    \
        T *data;         \
        size_t length;   \
        size_t capacity; \
    }

#define vec_init(v) memset((v), 0, sizeof(*(v)))

#define vec_push(v, val)                                          \
    vec_expand((void **)&(v)->data, &(v)->length, &(v)->capacity, \
               sizeof(*(v)->data));                               \
    (v)->data[(v)->length++] = (val)

#define vec_deinit(v) free((v)->data)

#define vec_pop(v) \
    (v)->data[--((v)->length)]

#endif /* LIB_GAIA_VEC_H */

#ifndef SRC_GAIA_RIGHTS_H
#define SRC_GAIA_RIGHTS_H
#include <gaia/vec.h>

#define RIGHT_NULL 0
#define RIGHT_IO (1 << 0)
#define RIGHT_DMA (1 << 1)
#define RIGHT_REGISTER_DMA (1 << 2)

typedef uint8_t Rights;

#endif

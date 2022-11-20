#ifndef SRC_GAIA_ERROR_H
#define SRC_GAIA_ERROR_H

#define ERR_SUCCESS 0
#define ERR_FORBIDDEN 1
#define ERR_INVALID_PARAMETERS 2
#define ERR_IN_USE 3
#define ERR_NOT_FOUND 4
#define ERR_NOT_IMPLEMENTED 5
#define ERR_OUT_OF_MEMORY 6
#define ERR_FAILED 7

inline char *error_to_str(int error)
{
    char *table[] = {
        [ERR_FORBIDDEN] = "Forbidden",
        [ERR_INVALID_PARAMETERS] = "Invalid parameters",
        [ERR_IN_USE] = "In use",
        [ERR_NOT_FOUND] = "Not found",
        [ERR_NOT_IMPLEMENTED] = "Not implemented",
        [ERR_OUT_OF_MEMORY] = "Out of memory",
        [ERR_FAILED] = "Failed",
    };

    return table[error];
}

#endif

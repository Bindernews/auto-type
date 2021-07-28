/*
Define the list of data sources here. Note that this is NOT a real header file
and should NOT be included anywhere other than decode.c.
*/

// references for external data
struct source_s {
    const uint32_t len;
    const uint8_t *data;
    const char *name;
};

#include "example1_bin.h"
#include "example2_bin.h"

// Number of sources (by also putting this in the array size we get a compile error if the value is incorrect)
#define SOURCES_SIZE 2
const struct source_s SOURCES[SOURCES_SIZE] = {
    { EXAMPLE1_ARRAY_SIZE, EXAMPLE1_ARRAY, "Main Data" },
    { EXAMPLE2_ARRAY_SIZE, EXAMPLE2_ARRAY, "Example data 2" },
};

#pragma once
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Init decoder state. Should call before using anything else.
 * @return 0 on success, nonzero on error
 */
int decode_init();

/**
 * Returns the number of sources this decoder supports.
 */
int decode_num_sources();

/**
 * Returns the name of the given source or NULL on an invalid index.
 */
const char *decode_source_name(int source);

/**
 * Select a decoder source and begin decoding.
 * @return 0 on success, nonzero on error
 */
int decode_begin(int source);

/**
 * Returns the next character for the currently selected source,
 * -1 on error, -2 on end of stream.
 */
int decode_next_char();

#ifdef __cplusplus
}
#endif

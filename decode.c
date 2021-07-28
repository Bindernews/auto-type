#include "miniz.h"
#include "decode.h"
#include <string.h>
#include <stdbool.h>
#include "sources.h"

#define LZ_BUF_SIZE (1024)

typedef struct decode_state_s {
    /** Output buffer for mz decoding */
    uint8_t outbuf[LZ_BUF_SIZE];
    /** Next index to return in outbuf. If this reaches LZ_BUF_SIZE - stream.avail_out, we need to fill the buffer again. */
    size_t outpos;
    /** Decoding stream struct */
    mz_stream stream;
    /** Are we done decoding the current stream? Also true if there is no stream. */
    bool done;
    /** Current stream index, -1 = no stream */
    int source;
} decode_t;

static decode_t s_decode_state;
// I'd rather use a pointer in case I have to make this a module later on
#define SELF (&s_decode_state)

int decode_init()
{
    SELF->done = true;
    SELF->source = -1;
    memset(&SELF->stream, 0, sizeof(mz_stream));
    return 0;
}

int decode_num_sources()
{
    return SOURCES_SIZE;
}

const char* decode_source_name(int source)
{
    if (source < 0 || source >= SOURCES_SIZE) {
        return NULL;
    }
    return SOURCES[source].name;
}

int decode_begin(int source)
{
    decode_t *self = SELF;
    if (source < 0 || source >= SOURCES_SIZE) {
        return 1;
    }

    // Make sure we're done with the previous decode
    if (self->source != -1) {
        mz_inflateEnd(&self->stream);
    }

    self->stream.next_in = SOURCES[source].data;
    self->stream.avail_in = SOURCES[source].len;
    self->stream.avail_out = LZ_BUF_SIZE;
    self->stream.next_out = self->outbuf;
    self->outpos = 0;
    self->source = source;
    self->done = false;

    // Make sure inflate inited correctly
    int r = mz_inflateInit(&self->stream);
    if (r == 0) {
        return 0;
    } else {
        self->done = true;
        self->source = -1;
    }

    return 1;
}

int decode_next_char()
{
    decode_t *self = SELF;
    if (self->done) {
        return -1;
    }
    if (self->outpos >= LZ_BUF_SIZE - self->stream.avail_out) {
        self->stream.next_out = self->outbuf;
        self->stream.avail_out = LZ_BUF_SIZE;
        int status = mz_inflate(&self->stream, MZ_NO_FLUSH);
        if (status == MZ_STREAM_END) {
            mz_inflateEnd(&self->stream);
            status = MZ_OK;
        }
        if (status != MZ_OK) {
            self->done = true;
            return -1;
        }
        self->outpos = 0;
    }
    if (self->outpos < LZ_BUF_SIZE - self->stream.avail_out) {
        uint8_t result = self->outbuf[self->outpos++];
        return result;
    }
    else {
        return -1;
    }
}

#include "bufopen.h"

#define INCREMENT 4096

struct buffer {
    char  *ptr;
    size_t used;
    size_t len;
    char **psave;
    size_t *plen;
};

typedef struct buffer BUFFER ;

/* called when data is written to the stream. len is the number of bytes
 * to read from data. The data buffer will not always include a terminating
 * null character. */
static int buffer_write(void *cookie, const char *data, int len) {
    int sz = len;
    BUFFER *buf = cookie;
    char *pbuf;

    /* allocate a buffer capable of storing n+1 bytes. */
    while ((buf->len - buf->used) <= len) {
        buf->len += INCREMENT;
        if ((buf->ptr = realloc(buf->ptr, buf->len)) == NULL)
            return -1;
    }

    pbuf = buf->ptr + buf->used;

    while (sz-- > 0)
        *pbuf++ = *data++;

    buf->used += len;

    /* add null terminator if last char + 1 isn't already null */
    if (*pbuf != '\0')
        *pbuf = '\0';

    return len;
}


/* called when the stream is closed */
static int buffer_close(void *cookie) {
    BUFFER *buf = cookie;
    *(buf->plen)  = buf->len;
    *(buf->psave) = buf->ptr;
    free(buf);
    return 0;
}

/* create a stream backed by a char buffer. most of the stream manipulation
 * functions (fprintf, putc, etc.) can be used directly against the stream
 * returned. The psave argument is a pointer to a char pointer that will
 * receive the final buffer allocation. The plen argument is a pointer to
 * a size_t that will receive the number of bytes written to the buffer.
 *
 * This function's interface was inspired by GNU's open_memstream. */
FILE *bufopen(size_t len, char ** psave, size_t * plen) {
    FILE *rv;

    BUFFER * buf;
    if ((buf = malloc(sizeof *buf)) == NULL)
        return NULL;
    buf->len = (len == 0) ? INCREMENT : len;
    buf->used = 0;
    buf->ptr = malloc(buf->len);
    buf->psave = psave;
    buf->plen = plen;

    rv = funopen(buf, NULL, buffer_write, NULL, buffer_close);
    if (rv == NULL)
        free(buf);
    return rv;
}

/* vim:set ts=4 sw=4: */

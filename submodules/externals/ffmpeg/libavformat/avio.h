/*
 * copyright (c) 2001 Fabrice Bellard
 *
 * This file is part of FFmpeg.
 *
 * FFmpeg is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * FFmpeg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with FFmpeg; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */
#ifndef AVFORMAT_AVIO_H
#define AVFORMAT_AVIO_H

/**
 * @file
 * unbuffered I/O operations
 *
 * @warning This file has to be considered an internal but installed
 * header, so it should not be directly included in your projects.
 */

#include <stdint.h>

#include "libavutil/common.h"
#include "libavutil/log.h"

#include "libavformat/version.h"

/* unbuffered I/O */

/**
 * URL Context.
 * New fields can be added to the end with minor version bumps.
 * Removal, reordering and changes to existing fields require a major
 * version bump.
 * sizeof(URLContext) must not be used outside libav*.
 */
typedef struct URLContext {
#if FF_API_URL_CLASS
    const AVClass *av_class; ///< information for av_log(). Set by url_open().
#endif
    struct URLProtocol *prot;
    int flags;
    int is_streamed;  /**< true if streamed (no seek possible), default = false */
    int max_packet_size;  /**< if non zero, the stream is packetized with this max packet size */
    void *priv_data;
    char *filename; /**< specified URL */
    int is_connected;
} URLContext;

typedef struct URLPollEntry {
    URLContext *handle;
    int events;
    int revents;
} URLPollEntry;

/**
 * @defgroup open_modes URL open modes
 * The flags argument to url_open and cosins must be one of the following
 * constants, optionally ORed with other flags.
 * @{
 */
#define URL_RDONLY 0  /**< read-only */
#define URL_WRONLY 1  /**< write-only */
#define URL_RDWR   2  /**< read-write */
/**
 * @}
 */

/**
 * Use non-blocking mode.
 * If this flag is set, operations on the context will return
 * AVERROR(EAGAIN) if they can not be performed immediately.
 * If this flag is not set, operations on the context will never return
 * AVERROR(EAGAIN).
 * Note that this flag does not affect the opening/connecting of the
 * context. Connecting a protocol will always block if necessary (e.g. on
 * network protocols) but never hang (e.g. on busy devices).
 * Warning: non-blocking protocols is work-in-progress; this flag may be
 * silently ignored.
 */
#define URL_FLAG_NONBLOCK 4

typedef int URLInterruptCB(void);

/**
 * Create a URLContext for accessing to the resource indicated by
 * url, and open it using the URLProtocol up.
 *
 * @param puc pointer to the location where, in case of success, the
 * function puts the pointer to the created URLContext
 * @param flags flags which control how the resource indicated by url
 * is to be opened
 * @return 0 in case of success, a negative value corresponding to an
 * AVERROR code in case of failure
 */
int url_open_protocol (URLContext **puc, struct URLProtocol *up,
                       const char *url, int flags);

/**
 * Create a URLContext for accessing to the resource indicated by
 * url, but do not initiate the connection yet.
 *
 * @param puc pointer to the location where, in case of success, the
 * function puts the pointer to the created URLContext
 * @param flags flags which control how the resource indicated by url
 * is to be opened
 * @return 0 in case of success, a negative value corresponding to an
 * AVERROR code in case of failure
 */
int url_alloc(URLContext **h, const char *url, int flags);

/**
 * Connect an URLContext that has been allocated by url_alloc
 */
int url_connect(URLContext *h);

/**
 * Create an URLContext for accessing to the resource indicated by
 * url, and open it.
 *
 * @param puc pointer to the location where, in case of success, the
 * function puts the pointer to the created URLContext
 * @param flags flags which control how the resource indicated by url
 * is to be opened
 * @return 0 in case of success, a negative value corresponding to an
 * AVERROR code in case of failure
 */
int url_open(URLContext **h, const char *url, int flags);

/**
 * Read up to size bytes from the resource accessed by h, and store
 * the read bytes in buf.
 *
 * @return The number of bytes actually read, or a negative value
 * corresponding to an AVERROR code in case of error. A value of zero
 * indicates that it is not possible to read more from the accessed
 * resource (except if the value of the size argument is also zero).
 */
int url_read(URLContext *h, unsigned char *buf, int size);

/**
 * Read as many bytes as possible (up to size), calling the
 * read function multiple times if necessary.
 * This makes special short-read handling in applications
 * unnecessary, if the return value is < size then it is
 * certain there was either an error or the end of file was reached.
 */
int url_read_complete(URLContext *h, unsigned char *buf, int size);

/**
 * Write size bytes from buf to the resource accessed by h.
 *
 * @return the number of bytes actually written, or a negative value
 * corresponding to an AVERROR code in case of failure
 */
int url_write(URLContext *h, const unsigned char *buf, int size);

/**
 * Passing this as the "whence" parameter to a seek function causes it to
 * return the filesize without seeking anywhere. Supporting this is optional.
 * If it is not supported then the seek function will return <0.
 */
#define AVSEEK_SIZE 0x10000

/**
 * Change the position that will be used by the next read/write
 * operation on the resource accessed by h.
 *
 * @param pos specifies the new position to set
 * @param whence specifies how pos should be interpreted, it must be
 * one of SEEK_SET (seek from the beginning), SEEK_CUR (seek from the
 * current position), SEEK_END (seek from the end), or AVSEEK_SIZE
 * (return the filesize of the requested resource, pos is ignored).
 * @return a negative value corresponding to an AVERROR code in case
 * of failure, or the resulting file position, measured in bytes from
 * the beginning of the file. You can use this feature together with
 * SEEK_CUR to read the current file position.
 */
int64_t url_seek(URLContext *h, int64_t pos, int whence);

/**
 * Close the resource accessed by the URLContext h, and free the
 * memory used by it.
 *
 * @return a negative value if an error condition occurred, 0
 * otherwise
 */
int url_close(URLContext *h);

/**
 * Return a non-zero value if the resource indicated by url
 * exists, 0 otherwise.
 */
int url_exist(const char *url);

/**
 * Return the filesize of the resource accessed by h, AVERROR(ENOSYS)
 * if the operation is not supported by h, or another negative value
 * corresponding to an AVERROR error code in case of failure.
 */
int64_t url_filesize(URLContext *h);

/**
 * Return the file descriptor associated with this URL. For RTP, this
 * will return only the RTP file descriptor, not the RTCP file descriptor.
 *
 * @return the file descriptor associated with this URL, or <0 on error.
 */
int url_get_file_handle(URLContext *h);

/**
 * Return the maximum packet size associated to packetized file
 * handle. If the file is not packetized (stream like HTTP or file on
 * disk), then 0 is returned.
 *
 * @param h file handle
 * @return maximum packet size in bytes
 */
int url_get_max_packet_size(URLContext *h);

/**
 * Copy the filename of the resource accessed by h to buf.
 *
 * @param buf_size size in bytes of buf
 */
void url_get_filename(URLContext *h, char *buf, int buf_size);

/**
 * The callback is called in blocking functions to test regulary if
 * asynchronous interruption is needed. AVERROR_EXIT is returned
 * in this case by the interrupted function. 'NULL' means no interrupt
 * callback is given.
 */
void url_set_interrupt_cb(URLInterruptCB *interrupt_cb);

/* not implemented */
int url_poll(URLPollEntry *poll_table, int n, int timeout);

/**
 * Pause and resume playing - only meaningful if using a network streaming
 * protocol (e.g. MMS).
 * @param pause 1 for pause, 0 for resume
 */
int av_url_read_pause(URLContext *h, int pause);

/**
 * Seek to a given timestamp relative to some component stream.
 * Only meaningful if using a network streaming protocol (e.g. MMS.).
 * @param stream_index The stream index that the timestamp is relative to.
 *        If stream_index is (-1) the timestamp should be in AV_TIME_BASE
 *        units from the beginning of the presentation.
 *        If a stream_index >= 0 is used and the protocol does not support
 *        seeking based on component streams, the call will fail with ENOTSUP.
 * @param timestamp timestamp in AVStream.time_base units
 *        or if there is no stream specified then in AV_TIME_BASE units.
 * @param flags Optional combination of AVSEEK_FLAG_BACKWARD, AVSEEK_FLAG_BYTE
 *        and AVSEEK_FLAG_ANY. The protocol may silently ignore
 *        AVSEEK_FLAG_BACKWARD and AVSEEK_FLAG_ANY, but AVSEEK_FLAG_BYTE will
 *        fail with ENOTSUP if used and not supported.
 * @return >= 0 on success
 * @see AVInputFormat::read_seek
 */
int64_t av_url_read_seek(URLContext *h, int stream_index,
                         int64_t timestamp, int flags);

/**
 * Oring this flag as into the "whence" parameter to a seek function causes it to
 * seek by any means (like reopening and linear reading) or other normally unreasonble
 * means that can be extreemly slow.
 * This may be ignored by the seek code.
 */
#define AVSEEK_FORCE 0x20000

#define URL_PROTOCOL_FLAG_NESTED_SCHEME 1 /*< The protocol name can be the first part of a nested protocol scheme */

typedef struct URLProtocol {
    const char *name;
    int (*url_open)(URLContext *h, const char *url, int flags);
    int (*url_read)(URLContext *h, unsigned char *buf, int size);
    int (*url_write)(URLContext *h, const unsigned char *buf, int size);
    int64_t (*url_seek)(URLContext *h, int64_t pos, int whence);
    int (*url_close)(URLContext *h);
    struct URLProtocol *next;
    int (*url_read_pause)(URLContext *h, int pause);
    int64_t (*url_read_seek)(URLContext *h, int stream_index,
                             int64_t timestamp, int flags);
    int (*url_get_file_handle)(URLContext *h);
    int priv_data_size;
    const AVClass *priv_data_class;
    int flags;
} URLProtocol;

#if FF_API_REGISTER_PROTOCOL
extern URLProtocol *first_protocol;
#endif

extern URLInterruptCB *url_interrupt_cb;

/**
 * If protocol is NULL, returns the first registered protocol,
 * if protocol is non-NULL, returns the next registered protocol after protocol,
 * or NULL if protocol is the last one.
 */
URLProtocol *av_protocol_next(URLProtocol *p);

#if FF_API_REGISTER_PROTOCOL
/**
 * @deprecated Use av_register_protocol() instead.
 */
attribute_deprecated int register_protocol(URLProtocol *protocol);

/**
 * @deprecated Use av_register_protocol2() instead.
 */
attribute_deprecated int av_register_protocol(URLProtocol *protocol);
#endif

/**
 * Register the URLProtocol protocol.
 *
 * @param size the size of the URLProtocol struct referenced
 */
int av_register_protocol2(URLProtocol *protocol, int size);

/**
 * Bytestream IO Context.
 * New fields can be added to the end with minor version bumps.
 * Removal, reordering and changes to existing fields require a major
 * version bump.
 * sizeof(AVIOContext) must not be used outside libav*.
 */
typedef struct {
    unsigned char *buffer;
    int buffer_size;
    unsigned char *buf_ptr, *buf_end;
    void *opaque;
    int (*read_packet)(void *opaque, uint8_t *buf, int buf_size);
    int (*write_packet)(void *opaque, uint8_t *buf, int buf_size);
    int64_t (*seek)(void *opaque, int64_t offset, int whence);
    int64_t pos; /**< position in the file of the current buffer */
    int must_flush; /**< true if the next seek should flush */
    int eof_reached; /**< true if eof reached */
    int write_flag;  /**< true if open for writing */
    int is_streamed;
    int max_packet_size;
    unsigned long checksum;
    unsigned char *checksum_ptr;
    unsigned long (*update_checksum)(unsigned long checksum, const uint8_t *buf, unsigned int size);
    int error;         ///< contains the error code or 0 if no error happened
    int (*read_pause)(void *opaque, int pause);
    int64_t (*read_seek)(void *opaque, int stream_index,
                         int64_t timestamp, int flags);
} AVIOContext;

#if FF_API_OLD_AVIO
typedef attribute_deprecated AVIOContext ByteIOContext;

attribute_deprecated int init_put_byte(AVIOContext *s,
                  unsigned char *buffer,
                  int buffer_size,
                  int write_flag,
                  void *opaque,
                  int (*read_packet)(void *opaque, uint8_t *buf, int buf_size),
                  int (*write_packet)(void *opaque, uint8_t *buf, int buf_size),
                  int64_t (*seek)(void *opaque, int64_t offset, int whence));
attribute_deprecated AVIOContext *av_alloc_put_byte(
                  unsigned char *buffer,
                  int buffer_size,
                  int write_flag,
                  void *opaque,
                  int (*read_packet)(void *opaque, uint8_t *buf, int buf_size),
                  int (*write_packet)(void *opaque, uint8_t *buf, int buf_size),
                  int64_t (*seek)(void *opaque, int64_t offset, int whence));

/**
 * @defgroup old_avio_funcs Old put_/get_*() functions
 * @deprecated use the avio_ -prefixed functions instead.
 * @{
 */
attribute_deprecated int          get_buffer(AVIOContext *s, unsigned char *buf, int size);
attribute_deprecated int          get_partial_buffer(AVIOContext *s, unsigned char *buf, int size);
attribute_deprecated int          get_byte(AVIOContext *s);
attribute_deprecated unsigned int get_le16(AVIOContext *s);
attribute_deprecated unsigned int get_le24(AVIOContext *s);
attribute_deprecated unsigned int get_le32(AVIOContext *s);
attribute_deprecated uint64_t     get_le64(AVIOContext *s);
attribute_deprecated unsigned int get_be16(AVIOContext *s);
attribute_deprecated unsigned int get_be24(AVIOContext *s);
attribute_deprecated unsigned int get_be32(AVIOContext *s);
attribute_deprecated uint64_t     get_be64(AVIOContext *s);

attribute_deprecated void         put_byte(AVIOContext *s, int b);
attribute_deprecated void         put_nbyte(AVIOContext *s, int b, int count);
attribute_deprecated void         put_buffer(AVIOContext *s, const unsigned char *buf, int size);
attribute_deprecated void         put_le64(AVIOContext *s, uint64_t val);
attribute_deprecated void         put_be64(AVIOContext *s, uint64_t val);
attribute_deprecated void         put_le32(AVIOContext *s, unsigned int val);
attribute_deprecated void         put_be32(AVIOContext *s, unsigned int val);
attribute_deprecated void         put_le24(AVIOContext *s, unsigned int val);
attribute_deprecated void         put_be24(AVIOContext *s, unsigned int val);
attribute_deprecated void         put_le16(AVIOContext *s, unsigned int val);
attribute_deprecated void         put_be16(AVIOContext *s, unsigned int val);
attribute_deprecated void         put_tag(AVIOContext *s, const char *tag);
/**
 * @}
 */

attribute_deprecated int     av_url_read_fpause(AVIOContext *h,    int pause);
attribute_deprecated int64_t av_url_read_fseek (AVIOContext *h,    int stream_index,
                                                int64_t timestamp, int flags);

/**
 * @defgroup old_url_f_funcs Old url_f* functions
 * @deprecated use the avio_ -prefixed functions instead.
 * @{
 */
attribute_deprecated int url_fopen( AVIOContext **s, const char *url, int flags);
attribute_deprecated int url_fclose(AVIOContext *s);
attribute_deprecated int64_t url_fseek(AVIOContext *s, int64_t offset, int whence);
attribute_deprecated int url_fskip(AVIOContext *s, int64_t offset);
attribute_deprecated int64_t url_ftell(AVIOContext *s);
attribute_deprecated int64_t url_fsize(AVIOContext *s);
#define URL_EOF (-1)
attribute_deprecated int url_fgetc(AVIOContext *s);
attribute_deprecated int url_setbufsize(AVIOContext *s, int buf_size);
#ifdef __GNUC__
attribute_deprecated int url_fprintf(AVIOContext *s, const char *fmt, ...) __attribute__ ((__format__ (__printf__, 2, 3)));
#else
attribute_deprecated int url_fprintf(AVIOContext *s, const char *fmt, ...);
#endif
attribute_deprecated void put_flush_packet(AVIOContext *s);
/**
 * @}
 */

attribute_deprecated int url_ferror(AVIOContext *s);

attribute_deprecated int udp_set_remote_url(URLContext *h, const char *uri);
attribute_deprecated int udp_get_local_port(URLContext *h);
#endif

AVIOContext *avio_alloc_context(
                  unsigned char *buffer,
                  int buffer_size,
                  int write_flag,
                  void *opaque,
                  int (*read_packet)(void *opaque, uint8_t *buf, int buf_size),
                  int (*write_packet)(void *opaque, uint8_t *buf, int buf_size),
                  int64_t (*seek)(void *opaque, int64_t offset, int whence));

void avio_w8(AVIOContext *s, int b);
void avio_write(AVIOContext *s, const unsigned char *buf, int size);
void avio_wl64(AVIOContext *s, uint64_t val);
void avio_wb64(AVIOContext *s, uint64_t val);
void avio_wl32(AVIOContext *s, unsigned int val);
void avio_wb32(AVIOContext *s, unsigned int val);
void avio_wl24(AVIOContext *s, unsigned int val);
void avio_wb24(AVIOContext *s, unsigned int val);
void avio_wl16(AVIOContext *s, unsigned int val);
void avio_wb16(AVIOContext *s, unsigned int val);

#if FF_API_OLD_AVIO
attribute_deprecated void put_strz(AVIOContext *s, const char *buf);
#endif

/**
 * Write a NULL-terminated string.
 * @return number of bytes written.
 */
int avio_put_str(AVIOContext *s, const char *str);

/**
 * Convert an UTF-8 string to UTF-16LE and write it.
 * @return number of bytes written.
 */
int avio_put_str16le(AVIOContext *s, const char *str);

/**
 * fseek() equivalent for AVIOContext.
 * @return new position or AVERROR.
 */
int64_t avio_seek(AVIOContext *s, int64_t offset, int whence);

/**
 * Skip given number of bytes forward
 * @return new position or AVERROR.
 */
int64_t avio_skip(AVIOContext *s, int64_t offset);

/**
 * ftell() equivalent for AVIOContext.
 * @return position or AVERROR.
 */
static av_always_inline int64_t avio_tell(AVIOContext *s)
{
    return avio_seek(s, 0, SEEK_CUR);
}

/**
 * Get the filesize.
 * @return filesize or AVERROR
 */
int64_t avio_size(AVIOContext *s);

/**
 * feof() equivalent for AVIOContext.
 * @return non zero if and only if end of file
 */
int url_feof(AVIOContext *s);

/** @warning currently size is limited */
#ifdef __GNUC__
int avio_printf(AVIOContext *s, const char *fmt, ...) __attribute__ ((__format__ (__printf__, 2, 3)));
#else
int avio_printf(AVIOContext *s, const char *fmt, ...);
#endif

#if FF_API_OLD_AVIO
/** @note unlike fgets, the EOL character is not returned and a whole
    line is parsed. return NULL if first char read was EOF */
attribute_deprecated char *url_fgets(AVIOContext *s, char *buf, int buf_size);
#endif

void avio_flush(AVIOContext *s);


/**
 * Read size bytes from AVIOContext into buf.
 * @return number of bytes read or AVERROR
 */
int avio_read(AVIOContext *s, unsigned char *buf, int size);

/** @note return 0 if EOF, so you cannot use it if EOF handling is
    necessary */
int          avio_r8  (AVIOContext *s);
unsigned int avio_rl16(AVIOContext *s);
unsigned int avio_rl24(AVIOContext *s);
unsigned int avio_rl32(AVIOContext *s);
uint64_t     avio_rl64(AVIOContext *s);

/**
 * Read a string from pb into buf. The reading will terminate when either
 * a NULL character was encountered, maxlen bytes have been read, or nothing
 * more can be read from pb. The result is guaranteed to be NULL-terminated, it
 * will be truncated if buf is too small.
 * Note that the string is not interpreted or validated in any way, it
 * might get truncated in the middle of a sequence for multi-byte encodings.
 *
 * @return number of bytes read (is always <= maxlen).
 * If reading ends on EOF or error, the return value will be one more than
 * bytes actually read.
 */
int avio_get_str(AVIOContext *pb, int maxlen, char *buf, int buflen);

/**
 * Read a UTF-16 string from pb and convert it to UTF-8.
 * The reading will terminate when either a null or invalid character was
 * encountered or maxlen bytes have been read.
 * @return number of bytes read (is always <= maxlen)
 */
int avio_get_str16le(AVIOContext *pb, int maxlen, char *buf, int buflen);
int avio_get_str16be(AVIOContext *pb, int maxlen, char *buf, int buflen);

#if FF_API_OLD_AVIO
/**
 * @deprecated use avio_get_str instead
 */
attribute_deprecated char *get_strz(AVIOContext *s, char *buf, int maxlen);
#endif
unsigned int avio_rb16(AVIOContext *s);
unsigned int avio_rb24(AVIOContext *s);
unsigned int avio_rb32(AVIOContext *s);
uint64_t     avio_rb64(AVIOContext *s);

static inline int url_is_streamed(AVIOContext *s)
{
    return s->is_streamed;
}

/**
 * Create and initialize a AVIOContext for accessing the
 * resource referenced by the URLContext h.
 * @note When the URLContext h has been opened in read+write mode, the
 * AVIOContext can be used only for writing.
 *
 * @param s Used to return the pointer to the created AVIOContext.
 * In case of failure the pointed to value is set to NULL.
 * @return 0 in case of success, a negative value corresponding to an
 * AVERROR code in case of failure
 */
int url_fdopen(AVIOContext **s, URLContext *h);

#if FF_API_URL_RESETBUF
/** Reset the buffer for reading or writing.
 * @note Will drop any data currently in the buffer without transmitting it.
 * @param flags URL_RDONLY to set up the buffer for reading, or URL_WRONLY
 *        to set up the buffer for writing. */
int url_resetbuf(AVIOContext *s, int flags);
#endif

/**
 * Create and initialize a AVIOContext for accessing the
 * resource indicated by url.
 * @note When the resource indicated by url has been opened in
 * read+write mode, the AVIOContext can be used only for writing.
 *
 * @param s Used to return the pointer to the created AVIOContext.
 * In case of failure the pointed to value is set to NULL.
 * @param flags flags which control how the resource indicated by url
 * is to be opened
 * @return 0 in case of success, a negative value corresponding to an
 * AVERROR code in case of failure
 */
int avio_open(AVIOContext **s, const char *url, int flags);

int avio_close(AVIOContext *s);

#if FF_API_OLD_AVIO
attribute_deprecated URLContext *url_fileno(AVIOContext *s);

/**
 * @deprecated use AVIOContext.max_packet_size directly.
 */
attribute_deprecated int url_fget_max_packet_size(AVIOContext *s);

attribute_deprecated int url_open_buf(AVIOContext **s, uint8_t *buf, int buf_size, int flags);

/** return the written or read size */
attribute_deprecated int url_close_buf(AVIOContext *s);
#endif

/**
 * Open a write only memory stream.
 *
 * @param s new IO context
 * @return zero if no error.
 */
int url_open_dyn_buf(AVIOContext **s);

/**
 * Open a write only packetized memory stream with a maximum packet
 * size of 'max_packet_size'.  The stream is stored in a memory buffer
 * with a big endian 4 byte header giving the packet size in bytes.
 *
 * @param s new IO context
 * @param max_packet_size maximum packet size (must be > 0)
 * @return zero if no error.
 */
int url_open_dyn_packet_buf(AVIOContext **s, int max_packet_size);

/**
 * Return the written size and a pointer to the buffer. The buffer
 * must be freed with av_free(). If the buffer is opened with
 * url_open_dyn_buf, then padding of FF_INPUT_BUFFER_PADDING_SIZE is
 * added; if opened with url_open_dyn_packet_buf, no padding is added.
 *
 * @param s IO context
 * @param pbuffer pointer to a byte buffer
 * @return the length of the byte buffer
 */
int url_close_dyn_buf(AVIOContext *s, uint8_t **pbuffer);

unsigned long ff_crc04C11DB7_update(unsigned long checksum, const uint8_t *buf,
                                    unsigned int len);
unsigned long get_checksum(AVIOContext *s);
void init_checksum(AVIOContext *s,
                   unsigned long (*update_checksum)(unsigned long c, const uint8_t *p, unsigned int len),
                   unsigned long checksum);

#if FF_API_UDP_GET_FILE
int udp_get_file_handle(URLContext *h);
#endif

#endif /* AVFORMAT_AVIO_H */

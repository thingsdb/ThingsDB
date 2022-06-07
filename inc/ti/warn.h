/*
 * ti/warn.h
 */
#ifndef TI_WARN_H_
#define TI_WARN_H_

/*
 * We give each possible warning a unique code > 0.
 */
typedef enum
{
    TI_WARN_THING_NOT_FOUND =1,
    TI_WARN_LOG_MESSAGE     =2,
} ti_warn_enum_t;

#include <ti/pkg.t.h>
#include <ti/proto.t.h>
#include <ti/raw.t.h>
#include <ti/stream.t.h>
#include <util/logger.h>
#include <util/mpack.h>
#include <stdarg.h>

int ti_warn(
        ti_stream_t * stream,
        uint16_t pid,
        ti_proto_enum_t proto,
        ti_warn_enum_t warn_code,
        const void * data,
        size_t n);

static inline int ti_warn_log_fwd(
        ti_stream_t * stream,
        uint16_t pid,
        const void * data,
        size_t n)
{
    return ti_warn(
            stream,
            pid,
            TI_PROTO_NODE_FWD_WARN,
            TI_WARN_LOG_MESSAGE,
            data,
            n);
}

static inline int ti_warn_log_client(
        ti_stream_t * stream,
        const void * data,
        size_t n)
{

    return ti_warn(
            stream,
            TI_PROTO_EV_ID,
            TI_PROTO_CLIENT_WARN,
            TI_WARN_LOG_MESSAGE,
            data,
            n);
}

#define ti_warn_fmt(stream__, tp__, fmt__, ...) \
do { \
    ti_pkg_t * pkg; \
    msgpack_packer pk; \
    msgpack_sbuffer buffer; \
    if (!ti_stream_is_client(stream__) && !ti_stream_is_closed(stream__)) \
        log_warning(fmt__, ##__VA_ARGS__); \
    else if (mp_sbuffer_alloc_init(&buffer, 1024, sizeof(ti_pkg_t)) == 0) { \
        msgpack_packer_init(&pk, &buffer, msgpack_sbuffer_write); \
        msgpack_pack_map(&pk, 2); \
        mp_pack_str(&pk, "warn_code"); \
        msgpack_pack_int8(&pk, tp__); \
        mp_pack_str(&pk, "warn_msg"); \
        if (mp_pack_fmt(&pk, fmt__, ##__VA_ARGS__) == 0) { \
            pkg = (ti_pkg_t *) buffer.data; \
            pkg_init(pkg, TI_PROTO_EV_ID, TI_PROTO_CLIENT_WARN, buffer.size); \
            (void) ti_stream_write_pkg(stream__, pkg); \
        } \
    } \
} while(0)

#endif  /* TI_WARN_H_ */

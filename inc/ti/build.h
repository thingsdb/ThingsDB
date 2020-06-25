/*
 * ti/build.h
 */
#ifndef TI_BUILD_H_
#define TI_BUILD_H_

enum
{
    TI_BUILD_WAITING,
    TI_BUILD_REQ_SETUP,
};

typedef struct ti_build_s ti_build_t;

#include <ti/stream.t.h>
#include <util/omap.h>

int ti_build_create(void);
void ti_build_destroy(void);
int ti_build_setup(
        uint32_t this_node_id,
        uint32_t from_node_id,
        uint8_t from_node_status,
        uint8_t from_node_zone,
        uint16_t from_node_syntax_ver,
        uint16_t from_node_port,
        ti_stream_t * stream);

struct ti_build_s
{
    uint32_t this_node_id;
    uint32_t from_node_id;
    uint8_t status;
    uint8_t from_node_status;
    uint8_t from_node_zone;
    uint16_t from_node_syntax_ver;
    uint16_t from_node_port;
};

#endif  /* TI_BUILD_H */

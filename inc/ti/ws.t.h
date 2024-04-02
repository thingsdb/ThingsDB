/*
 * ti/ws.t.h
 */
#ifndef TI_WS_T_H_
#define TI_WS_T_H_

typedef struct ti_ws_request_s ti_ws_request_t;

#define TI_WS_IDENTIFIER UINT32_MAX-1;

struct ti_ws_request_s
{
    uint32_t _id;               /* set to TI_API_IDENTIFIER */
    ti_scope_t scope;
    uv_write_t req;
    uv_stream_t uvstream;
    ex_t e;
    ti_user_t * user;
};

#endif  /* TI_WS_T_H_ */

/*
 * ti/api.t.h
 */
#ifndef TI_API_T_H_
#define TI_API_T_H_

typedef struct ti_api_request_s ti_api_request_t;

#define TI_API_IDENTIFIER UINT32_MAX-1;

typedef enum
{
    TI_API_CT_TEXT_PLAIN,
    TI_API_CT_TEXT_HTML,
    TI_API_CT_JSON,
    TI_API_CT_MSGPACK,
} ti_api_content_t;

typedef enum
{
    TI_API_STATE_NONE,
    TI_API_STATE_CONTENT_TYPE,
    TI_API_STATE_AUTHORIZATION,
} ti_api_state_t;

typedef enum
{
    TI_API_FLAG_IS_CLOSED       =1<<0,
    TI_API_FLAG_IN_USE          =1<<1,
    TI_API_FLAG_INVALID_SCOPE   =1<<2,
    TI_API_FLAG_JSON_BEAUTY     =1<<3,
    TI_API_FLAG_JSON_UTF8       =1<<4,
    TI_API_FLAG_HOME            =1<<5,
} ti_api_flags_t;

#include <ex.h>
#include <inttypes.h>
#include <lib/http_parser.h>
#include <ti/scope.t.h>
#include <ti/user.t.h>
#include <uv.h>

struct ti_api_request_s
{
    uint32_t id;               /* set to TI_API_IDENTIFIER */
    ti_api_flags_t flags;
    ti_api_content_t content_type;
    ti_api_state_t state;
    ti_scope_t scope;
    uv_write_t req;
    uv_stream_t uvstream;
    http_parser parser;
    ex_t e;
    size_t content_n;
    char * content;
    char * collection_name;     /* temporary store the collection name */
    ti_user_t * user;
};


#endif  /* TI_API_T_H_ */

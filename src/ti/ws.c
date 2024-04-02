/*
 * ti/ws.c
 *
 * WebSockets support.
 */
#include <libwebsockets.h>
#include <ti.h>
#include <ti/ws.h>
#include <ti/user.t.h>
#include <util/logger.h>

static struct lws_context * ws__context;

typedef struct  /* ws__msg_t */
{
    void * payload; /* is malloc'd */
    size_t len;
    char binary;
    char first;
    char final;
} ws__msg_t;

typedef struct  /* ws__psd_t */
{
    buf_t buf;
    ti_user_t * user;
} ws__psd_t;

static void ws__msg_destroy(void * _msg)
{
    ws__msg_t * msg = _msg;

    free(msg->payload);
    msg->payload = NULL;
    msg->len = 0;
}

typedef struct {
    struct lws_context * context;
    struct lws_vhost * vhost;
    int * interrupted;
    int * options;
} ws__vhd_t;

// Callback function for WebSocket server messages
int ws__callback(
        struct lws * wsi,
        enum lws_callback_reasons reason,
        void * user,
        void * in,
        size_t len)
{
    ws__psd_t * pss = (ws__psd_t *) user;
    ws__vhd_t * vhd = (ws__vhd_t *) lws_protocol_vh_priv_get(
                lws_get_vhost(wsi),
                lws_get_protocol(wsi));

    const ws__msg_t * pmsg;
    ws__msg_t amsg;
    int m, n, flags;

    switch (reason)
    {
    case LWS_CALLBACK_PROTOCOL_INIT:
        LOGC("LWS_CALLBACK_PROTOCOL_INIT");
        vhd = lws_protocol_vh_priv_zalloc(
                lws_get_vhost(wsi),
                lws_get_protocol(wsi),
                sizeof(ws__vhd_t));
        if (!vhd)
            return -1;

        vhd->context = lws_get_context(wsi);
        vhd->vhost = lws_get_vhost(wsi);

        /* get the pointers we were passed in pvo */

        vhd->interrupted = (int *)lws_pvo_search(
            (const struct lws_protocol_vhost_options *)in,
            "interrupted")->value;
        vhd->options = (int *)lws_pvo_search(
            (const struct lws_protocol_vhost_options *)in,
            "options")->value;
        break;
    case LWS_CALLBACK_ESTABLISHED:
        LOGC("LWS_CALLBACK_ESTABLISHED");
        buf_init(&pss->buf);
        pss->user = NULL;
        break;
    case LWS_CALLBACK_SERVER_WRITEABLE:
        LOGC("LWS_CALLBACK_SERVER_WRITEABLE");

        flags = lws_write_ws_flags(
                pmsg->binary ? LWS_WRITE_BINARY : LWS_WRITE_TEXT,
                pmsg->first, pmsg->final);

        /* notice we allowed for LWS_PRE in the payload already */
        m = lws_write(
                wsi,
                ((unsigned char *) pmsg->payload) + LWS_PRE,
                pmsg->len,
                (enum lws_write_protocol)flags);

        if (m < (int) pmsg->len)
        {
            log_error("ERROR %d writing to ws socket", m);
            return -1;
        }

        log_debug("wrote %d: flags: 0x%x first: %d final %d",
                m, flags, pmsg->first, pmsg->final);
        /*
         * Workaround deferred deflate in pmd extension by only
         * consuming the fifo entry when we are certain it has been
         * fully deflated at the next WRITABLE callback.  You only need
         * this if you're using pmd.
         */

        lws_callback_on_writable(wsi);

        if (pss->flow_controlled &&
            (int)lws_ring_get_count_free_elements(pss->ring) > RING_DEPTH - 5)
        {
            lws_rx_flow_control(wsi, 1);
            pss->flow_controlled = 0;
        }

        if ((*vhd->options & 1) && pmsg && pmsg->final)
            pss->completed = 1;

        break;
    case LWS_CALLBACK_RECEIVE:
        LOGC("LWS_CALLBACK_RECEIVE: %4d (rpp %5d, first %d, "
              "last %d, bin %d, msglen %d (+ %d = %d))\n",
              (int)len, (int)lws_remaining_packet_payload(wsi),
              lws_is_first_fragment(wsi),
              lws_is_final_fragment(wsi),
              lws_frame_is_binary(wsi), pss->msglen, (int)len,
              (int)pss->msglen + (int)len);

        amsg.first = (char) lws_is_first_fragment(wsi);
        amsg.final = (char) lws_is_final_fragment(wsi);
        amsg.binary = (char) lws_frame_is_binary(wsi);
        n = (int) lws_ring_get_count_free_elements(pss->ring);
        if (!n) {
            log_info("dropping!");
            break;
        }

        if (amsg.final)
            pss->msglen = 0;
        else
            pss->msglen += (uint32_t)len;

        amsg.len = len;
        /* notice we over-allocate by LWS_PRE */
        amsg.payload = malloc(LWS_PRE + len);
        if (!amsg.payload)
        {
            log_info("OOM: dropping");
            break;
        }

        memcpy((char *) amsg.payload + LWS_PRE, in, len);
        if (!lws_ring_insert(pss->ring, &amsg, 1))
        {
            ws__msg_destroy(&amsg);
            log_info("dropping!!");
            break;
        }

        lws_callback_on_writable(wsi);

        if (n < 3 && !pss->flow_controlled)
        {
            pss->flow_controlled = 1;
            lws_rx_flow_control(wsi, 0);
        }
        break;
    case LWS_CALLBACK_CLOSED:
        LOGC("LWS_CALLBACK_CLOSED");
        lws_ring_destroy(pss->ring);
        ti_user_drop(pss->user);
        break;
    default:
        break;
    }
    return 0;
}

static struct lws_protocols ws__protocols[] = {
    {
        .name                  = "thingsdb-protocol",/* Protocol name*/
        .callback              = ws__callback,       /* Protocol callback */
        .per_session_data_size = sizeof(ws__psd_t),  /* Protocol callback 'userdata' size */
        .rx_buffer_size        = 0,                  /* Receve buffer size (0 = no restriction) */
        .id                    = 0,                  /* Protocol Id (version) (optional) */
        .user                  = NULL,               /* 'User data' ptr, to access in 'protocol callback */
        .tx_packet_size        = 0                   /* Transmission buffer size restriction (0 = no restriction) */
    },
    { NULL, NULL, 0, 0, 0, NULL, 0 } // Protocol list ends with NULL
};

static int interrupted, options;

/* pass pointers to shared vars to the protocol */

static const struct lws_protocol_vhost_options pvo_options = {
    NULL,
    NULL,
    "options",      /* pvo name */
    (void *)&options    /* pvo value */
};

static const struct lws_protocol_vhost_options pvo_interrupted = {
    &pvo_options,
    NULL,
    "interrupted",      /* pvo name */
    (void *)&interrupted    /* pvo value */
};

static const struct lws_protocol_vhost_options pvo = {
    NULL,               /* "next" pvo linked-list */
    &pvo_interrupted,       /* "child" pvo linked-list */
    "thingsdb-protocol",  /* protocol name we belong to on this vhost */
    ""              /* ignored */
};

void ws__log(int level, const char *line)
{
    switch(level)
    {
    case LLL_ERR:       log_line(LOGGER_INFO, line);        break;
    case LLL_WARN:      log_line(LOGGER_WARNING, line);     break;
    case LLL_DEBUG:     log_line(LOGGER_DEBUG, line);       break;
    case LLL_USER:
    case LLL_NOTICE:    log_line(LOGGER_INFO, line);        break;
    }
}

int ti_ws_init()
{
    void *foreign_loops[1];
    struct lws_context_creation_info info;
    int logs = 0;

    switch (Logger.level)
    {
    /* for LLL_ verbosity above NOTICE to be built into lws,
     * lws must have been configured and built with
     * -DCMAKE_BUILD_TYPE=DEBUG instead of =RELEASE */
    /* | LLL_INFO */ /* | LLL_PARSER */ /* | LLL_HEADER */
    /* | LLL_EXT */ /* | LLL_CLIENT */ /* | LLL_LATENCY */
    /* | LLL_DEBUG */
    case LOGGER_DEBUG:
        // logs |= LLL_DEBUG;
        /*fall through*/
    case LOGGER_INFO:
        logs |= LLL_USER | LLL_NOTICE;
        /*fall through*/
    case LOGGER_WARNING:
        logs |= LLL_WARN;
        /*fall through*/
    case LOGGER_ERROR: logs |= LLL_ERR;
    }

    foreign_loops[0] = ti.loop;
    memset(&info, 0, sizeof info); /* otherwise uninitialized garbage */

    info.port = (int) ti.cfg->ws_port;
    if (info.port == 0)
        return 0;

    info.protocols = ws__protocols;
    info.pvo = &pvo;
    info.foreign_loops = foreign_loops;
    info.options = \
        LWS_SERVER_OPTION_DO_SSL_GLOBAL_INIT |
        LWS_SERVER_OPTION_HTTP_HEADERS_SECURITY_BEST_PRACTICES_ENFORCE |
        LWS_SERVER_OPTION_LIBUV;

#if defined(LWS_WITH_TLS)
    if (0 && ti.cfg->ws_cert_file && ti.cfg->ws_key_file) {
        log_info("WebSockets server using TLS");
        info.options |= LWS_SERVER_OPTION_DO_SSL_GLOBAL_INIT;
        info.ssl_cert_filepath = ti.cfg->ws_cert_file;
        info.ssl_private_key_filepath = ti.cfg->ws_key_file;
    }
#endif

    lws_set_log_level(logs, &ws__log);

    ws__context = lws_create_context(&info);
    if (!ws__context)
    {
        log_error("lws init failed");
        return -1;
    }

//    while (1) {
//            lws_service(ws__context, 50);
//        }

    return 0;
}

void ti_ws_destroy(void)
{
    if (ws__context)
        lws_context_destroy(ws__context);
    ws__context = NULL;
}

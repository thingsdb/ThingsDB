/*
 * signals.c
 */
#include <uv.h>
#include <signal.h>
#include <stdlib.h>
#include <ti/signals.h>
#include <ti.h>
#include <util/logger.h>

static void ti__signals_handler(uv_signal_t * sig, int signum);

#define nsigs 6
static const int signms[nsigs] = {
        SIGHUP,
        SIGINT,
        SIGTERM,
        SIGSEGV,
        SIGABRT,
        SIGPIPE
};

static uv_signal_t signals[nsigs] = {0};

int ti_signals_init(void)
{
    /* bind signals to the event loop */
    for (int i = 0; i < nsigs; i++)
    {
        if (uv_signal_init(ti_get()->loop, &signals[i]) ||
            uv_signal_start(&signals[i], ti__signals_handler, signms[i]))
        {
            return -1;
        }
    }
    return 0;
}

static void ti__signals_handler(uv_signal_t * UNUSED(sig), int signum)
{
    ti_t * thingsdb = ti_get();

    if (signum == SIGPIPE)
    {
        log_warning("signal (%d) received, probably a connection was lost");
        return;
    }

    if (thingsdb->flags & TI_FLAG_SIGNAL)
    {
        log_error("received second signal (%s), abort", strsignal(signum));
        abort();
    }
    thingsdb->flags |= TI_FLAG_SIGNAL;

    if (signum == SIGINT || signum == SIGTERM || signum == SIGHUP)
    {
        log_warning("received stop signal (%s)", strsignal(signum));
    }
    else
    {
        log_critical("received stop signal (%s)", strsignal(signum));
    }

    ti_maint_stop(thingsdb->maint);

    uv_stop(thingsdb->loop);
}

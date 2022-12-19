/*
 * ti/signals.c
 */
#include <signal.h>
#include <stdlib.h>
#include <ti.h>
#include <ti/away.h>
#include <ti/changes.h>
#include <ti/connect.h>
#include <ti/signals.h>
#include <util/logger.h>
#include <uv.h>

static void signals__handler(uv_signal_t * sig, int signum);

#define signals__nsigs 5
static const int signals__signms[signals__nsigs] = {
        SIGHUP,
        SIGINT,
        SIGTERM,
        SIGSEGV,
        SIGPIPE
};

static uv_signal_t signals[signals__nsigs];

int ti_signals_init(void)
{
    /* bind signals to the libuv event loop */
    for (int i = 0; i < signals__nsigs; i++)
    {
        if (uv_signal_init(ti.loop, &signals[i]) ||
            uv_signal_start(&signals[i], signals__handler, signals__signms[i]))
        {
            return -1;
        }
    }
    return 0;
}

static void signals__handler(uv_signal_t * UNUSED(sig), int signum)
{
    if (signum == SIGPIPE)
    {
        log_warning(
            "signal (%s) received, probably a connection was lost",
            strsignal(signum));
        return;
    }

    if (ti_flag_test(TI_FLAG_SIGNAL))
    {
        log_warning(
            "signal (%s) received but a shutdown is already initiated",
            strsignal(signum));
        if (signum == SIGSEGV)
            abort();
        return;
    }

    ti_flag_set(TI_FLAG_SIGNAL);

    if (signum == SIGINT || signum == SIGTERM || signum == SIGHUP)
        log_warning("received stop signal (%s)", strsignal(signum));
    else
        log_critical("received stop signal (%s)", strsignal(signum));

    if (ti.node->status != TI_NODE_STAT_AWAY)
        ti_shutdown();
}

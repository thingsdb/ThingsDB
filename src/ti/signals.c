/*
 * signals.c
 *
 *  Created on: Sep 29, 2017
 *      Author: Jeroen van der Heijden <jeroen@transceptor.technology>
 */
#include <uv.h>
#include <thingsdb.h>
#include <signal.h>
#include <stdlib.h>
#include <ti/signals.h>
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
        if (uv_signal_init(thingsdb_loop(), &signals[i]) ||
            uv_signal_start(&signals[i], ti__signals_handler, signms[i]))
        {
            return -1;
        }
    }
    return 0;
}

static void ti__signals_handler(uv_signal_t * sig, int signum)
{
    thingsdb_t * thingsdb = thingsdb_get();

    if (signum == SIGPIPE)
    {
        log_warning("signal (%d) received, probably a connection was lost");
        return;
    }

    log_warning("received stop signal (%s)", strsignal(signum));

    if (thingsdb->flags & TI_FLAG_SIGNAL)
    {
        abort();
    }

    thingsdb->flags |= TI_FLAG_SIGNAL;

    ti_maint_stop(thingsdb->maint);

    if (signum == SIGINT || signum == SIGTERM || signum == SIGHUP)
    {
        log_warning("received stop signal (%s)", strsignal(signum));
    }

    uv_stop(thingsdb->loop);
}

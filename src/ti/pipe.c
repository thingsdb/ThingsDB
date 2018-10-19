/*
 * pipe.c
 */
#include <stdlib.h>
#include <ti/pipe.h>
#include <util/fx.h>

/*
 * Return a name for the connection if successful or NULL in case of a failure.
 *
 * The returned value is malloced and should be freed.
 */
char * ti_pipe_name(uv_pipe_t * client)
{
    size_t len = FX_PATH_MAX - 1;
    char * buffer = malloc(FX_PATH_MAX);

    if (buffer == NULL || uv_pipe_getsockname(client, buffer, &len))
    {
        free(buffer);
        return NULL;
    }

    buffer[len] = '\0';
    return buffer;
}

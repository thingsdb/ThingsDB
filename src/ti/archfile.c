/*
 * ti/archfile.c
 */
#include <ti/archfile.h>
#include <ti.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <stdbool.h>
#include <string.h>
#include <util/fx.h>
#include <util/util.h>

/*
 * Archive file format: < hex_first_event >_< hex_last_event >.mp
 * Both the first and last event are inclusive inside the file.
 */
#define ARCHIVE__FILE_FMT "%016"PRIx64"_%016"PRIx64".mp"
/*
 * Archive file length (exclusive a terminator character).
 */
#define ARCHIVE__FILE_LEN 36

ti_archfile_t * ti_archfile_upsert(const char * path, const char * fn)
{
    /* Read the first and last event id from file name,
     * using format 16 hex digits + underscore + 16 hex digits */
    uint64_t first = strtoull(fn, NULL, 16);
    uint64_t last = strtoull(fn + 16 + 1, NULL, 16);
    ti_archfile_t * archfile = ti_archfile_get(first, last);
    if (archfile)
        return archfile;

    archfile = malloc(sizeof(ti_archfile_t));
    if (!archfile)
        return NULL;

    archfile->first = first;
    archfile->last = last;
    archfile->fn = fx_path_join(path, fn);

    if (!archfile->fn || vec_push(&ti.archive->archfiles, archfile))
    {
        ti_archfile_destroy(archfile);
        return NULL;
    }

    return archfile;
}

ti_archfile_t * ti_archfile_from_event_ids(
        const char * path,
        uint64_t first,
        uint64_t last)
{
    char buf[ARCHIVE__FILE_LEN + 1];
    ti_archfile_t * archfile = malloc(sizeof(ti_archfile_t));
    if (!archfile)
        return NULL;

    sprintf(buf, ARCHIVE__FILE_FMT, first, last);

    archfile->fn = fx_path_join(path, buf);
    if (!archfile->fn)
    {
        free(archfile);
        return NULL;
    }

    archfile->first = first;
    archfile->last = last;

    return archfile;
}

void ti_archfile_destroy(ti_archfile_t * archfile)
{
    if (!archfile)
        return;
    free(archfile->fn);
    free(archfile);
}

ti_archfile_t * ti_archfile_get(uint64_t first, uint64_t last)
{
    vec_t * archfiles = ti.archive->archfiles;
    for (vec_each(archfiles, ti_archfile_t, archfile))
        if (archfile->first == first && archfile->last == last)
            return archfile;
    return NULL;
}

_Bool ti_archfile_is_valid_fn(const char * fn)
{
    size_t len = strlen(fn);
    if (len != ARCHIVE__FILE_LEN)
        return false;

    /* twenty digits as start event_id */
    for (size_t i = 0; i < 16; ++i, ++fn)
        if (!isxdigit(*fn))
            return false;

    /* underscore */
    if (*fn != '_')
         return false;

    ++fn;

    /* twenty digits as last event_id */
    for (size_t i = 0; i < 16; ++i, ++fn)
        if (!isxdigit(*fn))
            return false;

    /* should end with `mp` extension */
    return strcmp(fn, ".mp") == 0;
}

/*
 * ti/archfile.h
 */
#ifndef TI_ARCHFILE_H_
#define TI_ARCHFILE_H_

typedef struct ti_archfile_s ti_archfile_t;

#include <inttypes.h>

ti_archfile_t * ti_archfile_create(const char * path, const char * fn);
ti_archfile_t * ti_archfile_from_event_ids(
        const char * path,
        uint64_t first,
        uint64_t last);
void ti_archfile_destroy(ti_archfile_t * archfile);
_Bool ti_archfile_is_valid_fn(const char * fn);


struct ti_archfile_s
{
    uint64_t first;
    uint64_t last;
    char * fn;
};

#endif  /* TI_ARCHFILE_H_ */

#include <ti/dump.h>
#include <tiinc.h>

int dump__mark_new(ti_thing_t * thing, void * UNUSED(arg))
{
    ti_thing_mark_new(thing);
    return 0;
}

int dump__unmark_new(ti_thing_t * thing, void * UNUSED(arg))
{
    ti_thing_unmark_new(thing);
    return 0;
}



int ti_dump_collection(ti_collection_t * collection, const char * fn)
{
    msgpack_packer pk;
    FILE * f = fopen(fn, "w");
    if (!f)
    {
       log_errno_file("cannot open file", errno, fn);
       return -1;
    }

    msgpack_packer_init(&pk, f, msgpack_fbuffer_write);

    ti_sleep(50);
    imap_walk(collection->things, (imap_cb) dump__mark_new, NULL);
    ti_sleep(50);


    log_debug("stored "TI_COLLECTION_ID" to file: `%s`", collection->id, fn);
    goto done;
fail:
    log_error("failed to write file: `%s`", fn);
done:
    ti_sleep(50);
    imap_walk(collection->things, (imap_cb) ti_thing_unmark_new, NULL);
    ti_sleep(50);

    if (fclose(f))
    {
       log_errno_file("cannot close file", errno, fn);
       return -1;
    }

    return 0;
}

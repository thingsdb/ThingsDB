/*
 * syncpart.c
 */
#include <errno.h>
#include <ex.h>
#include <stdio.h>
#include <stdlib.h>
#include <util/logger.h>
#include <util/syncpart.h>

/*
 * Returns 0 if the file is complete, 1 if more data is available and -1 on
 * error.
 */
int syncpart_to_pk(msgpack_packer * pk, const char * fn, off_t offset)
{
    int more;
    size_t sz;
    off_t restsz;
    unsigned char * buff;
    FILE * fp = fopen(fn, "r");
    if (!fp)
    {
        log_critical("cannot open file `%s` (%s)", fn, strerror(errno));
        goto fail0;
    }

    if (fseeko(fp, 0, SEEK_END) == -1 || (restsz = ftello(fp)) == -1)
    {
        log_critical("error seeking file `%s` (%s)", fn, strerror(errno));
        goto fail1;
    }

    if (offset > restsz)
    {
        log_critical("got an illegal offset for file `%s` (%zd)", fn, offset);
        goto fail1;
    }

    restsz -= offset;

    sz = (size_t) restsz > SYNCPART_SIZE ? SYNCPART_SIZE : (size_t) restsz;

    buff = malloc(sz);
    if (!buff)
    {
        log_critical(EX_MEMORY_S);
        goto fail1;
    }

    if (fseeko(fp, offset, SEEK_SET) == -1)
    {
        log_critical("error seeking file `%s` (%s)", fn, strerror(errno));
        goto fail2;
    }

    if (fread(buff, sizeof(char), sz, fp) != sz)
    {
        log_critical("cannot read %zu bytes from file `%s`", sz, fn);
        goto fail2;
    }

    if (fclose(fp))
    {
        log_critical("cannot close file `%s` (%s)", fn, strerror(errno));
        goto fail2;
    }

    more = mp_pack_bin(pk, buff, sz) ? -1 : (size_t) restsz != sz;
    free(buff);
    return more;

fail2:
    free(buff);
fail1:
    (void) fclose(fp);
fail0:
    return -1;
}

int syncpart_write(
        const char * fn,
        const unsigned char * data,
        size_t size,
        off_t offset,
        ex_t * e)
{
    off_t sz;
    FILE * fp;

    fp = fopen(fn, offset ? "ab" : "wb");
    if (!fp)
    {
        /* lock is required for use of strerror */
        uv_mutex_lock(&Logger.lock);
        ex_set(e, EX_INTERNAL,
                "cannot open file `%s` (%s)",
                fn, strerror(errno));
        uv_mutex_unlock(&Logger.lock);
        return e->nr;
    }
    sz = ftello(fp);
    if (sz != offset)
    {
        uv_mutex_lock(&Logger.lock);
        ex_set(e, EX_BAD_DATA,
                "file `%s` is expected to have size %zd (got: %zd, %s)",
                fn,
                offset,
                sz,
                sz == -1 ? strerror(errno) : "file size is different");
        uv_mutex_unlock(&Logger.lock);
        goto done;
    }

    if (fwrite(data, sizeof(char), size, fp) != size)
    {
        ex_set(e, EX_INTERNAL, "error writing %zu bytes to file `%s`",
                size, fn);
    }

done:
    if (fclose(fp) && !e->nr)
    {
        uv_mutex_lock(&Logger.lock);
        ex_set(e, EX_INTERNAL, "cannot close file `%s` (%s)",
                fn, strerror(errno));
        uv_mutex_unlock(&Logger.lock);
    }

    return e->nr;
}


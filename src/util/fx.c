/*
 * fx.c
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <stdbool.h>
#include <util/fx.h>
#include <util/logger.h>
#include <errno.h>

int fx_write(const char * fn, unsigned char * data, size_t n)
{
    int rc = 0;
    FILE * fp = fopen(fn, "w");
    if (!fp)
    {
        log_error("cannot open file `%s` (%s)", fn, strerror(errno));
        return -1;
    }

    if (fwrite(data, n, 1, fp) != 1)
    {
        log_error("cannot write %zu bytes to `%s`", n, fn);
        rc = -1;
    }

    if (fclose(fp))
    {
        log_error("cannot close file `%s` (%s)", fn, strerror(errno));
        rc = -1;
    }

    return rc;
}

unsigned char * fx_read(const char * fn, ssize_t * size)
{
    unsigned char * data = NULL;
    FILE * fp = fopen(fn, "r");
    if (!fp)
    {
        log_error("cannot open file `%s` (%s)", fn, strerror(errno));
        return NULL;
    }

    if (fseeko(fp, 0, SEEK_END) ||
        (*size = ftello(fp)) < 0  ||
        fseeko(fp, 0, SEEK_SET))
    {
        log_error("cannot read size of file `%s` (%s)", fn, strerror(errno));
        goto final;
    }

    data = malloc(*size);
    if (!data)
    {
        log_error("allocation error in `%s` at %s:%d",
                __func__, __FILE__, __LINE__);
        goto final;
    }

    if (fread(data, *size, 1, fp) != 1)
    {
        log_error("cannot read %zu bytes from file `%s`", *size, fn);
        free(data);
        data = NULL;
    }

final:
    if (fclose(fp))
        log_error("cannot close file `%s` (%s)", fn, strerror(errno));

    return data;
}

_Bool fx_file_exist(const char * fn)
{
    FILE * fp;
    fp = fopen(fn, "r");
    if (!fp)
        return false;
    (void) fclose(fp);
    return true;
}

_Bool fx_is_dir(const char * path)
{
    struct stat st = {0};
    return !stat(path, &st) && S_ISDIR(st.st_mode);
}

int fx_rmdir(const char * path)
{
    DIR * d = opendir(path);
    if (!d)
        return -1;

    size_t bufsz = 0, path_len = strlen(path);
    const char * slash = (path[path_len - 1] == '/') ? "" : "/";
    struct dirent * p;
    char * buf = NULL;

    while ((p = readdir(d)))
    {
        size_t len;

        /* Skip the names "." and ".." as we don't want to recurse on them. */
        if (!strcmp(p->d_name, ".") || !strcmp(p->d_name, ".."))
            continue;

        len = path_len + strlen(p->d_name) + 2;
        if (len > bufsz)
        {
            bufsz = len;
            char * tmp = realloc(buf, bufsz);
            if (!tmp) goto stop;
            buf = tmp;
        }

        snprintf(buf, len, "%s%s%s", path, slash, p->d_name);

        if (fx_is_dir(buf) ? fx_rmdir(buf) : unlink(buf))
            goto stop;
    }

stop:
    free(buf);
    closedir(d);

    return rmdir(path);
}

char * fx_path_join(const char * s1, const char * s2)
{
    size_t n1 = strlen(s1);
    size_t n2 = strlen(s2);
    int add_slash = (s1[n1-1] != '/');
    char * s = malloc(n1 + n2 + 1 + add_slash);
    if (!s)
        return NULL;

    memcpy(s, s1, n1);
    memcpy(s + n1 + add_slash, s2, n2 + 1);

    if (add_slash)
        s[n1] = '/';

    return s;
}

char * fx_get_exec_path(void)
{
    int rc;
    size_t buffer_sz = 1024;
    char * tmp, * buffer;

    buffer = malloc(buffer_sz);
    if (!buffer)
        goto failed;

    while (1)
    {
        rc = readlink("/proc/self/exe", buffer, buffer_sz);
        if (rc < 0)
            goto failed;
        if (rc < (ssize_t) buffer_sz) break;
        buffer_sz *= 2;
        tmp = realloc(buffer, buffer_sz);
        if (!tmp)
            goto failed;
        buffer = tmp;
    }

    /* find last / in path */
    tmp = strrchr(buffer, '/');
    if (!tmp)
        goto failed; /* no slash found in path */

    *(++tmp) = '\0';
    return buffer;

failed:
    free(buffer);
    return NULL;
}

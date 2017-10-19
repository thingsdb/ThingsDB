/*
 * filex.c
 *
 *  Created on: Sep 29, 2017
 *      Author: Jeroen van der Heijden <jeroen@transceptor.technology>
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <util/fx.h>

int fx_write(const char * fn, unsigned char * data, size_t n)
{
    int rc = 0;
    FILE * fp = fopen(fn, "w");
    if (!fp) return -1;

    rc = (fwrite(data, n, 1, fp) == 1) ? 0 : -1;

    return fclose(fp) || rc;
}

unsigned char * fx_read(const char * fn, ssize_t * size)
{
    unsigned char * data = NULL;
    FILE * fp = fopen(fn, "r");
    if (!fp) return NULL;
    if (fseeko(fp, 0, SEEK_END) ||
        (*size = ftello(fp)) < 0  ||
        fseeko(fp, 0, SEEK_SET)) goto final;
    data = (unsigned char *) malloc(*size);
    if (!data) goto final;
    if (fread(data, *size, 1, fp) != 1)
    {
        free(data);
        data = NULL;
    }

final:
    fclose(fp);
    return data;
}

_Bool fx_file_exist(const char * fn)
{
    FILE * fp;
    fp = fopen(fn, "r");
    if (!fp) return 0;
    fclose(fp);
    return 1;
}

_Bool fx_is_dir(const char * path)
{
    struct stat st = {0};
    return !stat(path, &st) && S_ISDIR(st.st_mode);
}

int fx_rmdir(const char * path)
{
    DIR * d = opendir(path);
    if (!d) return -1;

    size_t bufsz = 0, path_len = strlen(path);
    const char * slash = (path[path_len - 1] == '/') ? "" : "/";
    struct dirent * p;
    char * buf = NULL;

    while ((p = readdir(d)))
    {
        size_t len;

        /* Skip the names "." and ".." as we don't want to recurse on them. */
        if (!strcmp(p->d_name, ".") || !strcmp(p->d_name, "..")) continue;

        len = path_len + strlen(p->d_name) + 2;
        if (len > bufsz)
        {
            bufsz = len;
            char * tmp = (char *) realloc(buf, bufsz);
            if (!tmp) goto stop;
            buf = tmp;
        }

        snprintf(buf, len, "%s%s%s", path, slash, p->d_name);

        if (fx_is_dir(buf) ? fx_rmdir(buf) : unlink(buf)) goto stop;
    }

stop:
    free(buf);
    closedir(d);

    return rmdir(path);
}



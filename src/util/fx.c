/*
 * filex.c
 *
 *  Created on: Sep 29, 2017
 *      Author: Jeroen van der Heijden <jeroen@transceptor.technology>
 */
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
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

int fx_file_exist(const char * fn)
{
    FILE * fp;
    fp = fopen(fn, "r");
    if (!fp) return 0;
    fclose(fp);
    return 1;
}

int fx_is_dir(const char * path)
{
    struct stat st = {0};
    stat(path, &st);
    return S_ISDIR(st.st_mode);
}



/*
 * filex.c
 *
 *  Created on: Sep 29, 2017
 *      Author: Jeroen van der Heijden <jeroen@transceptor.technology>
 */
#include <stdio.h>

int filex_write(const char * fn, unsigned char * data, size_t n)
{
    int rc = 0;
    FILE * fp = fopen(fn, "w");
    if (!fp) return -1;

    rc = write(data, n, 1, fp);

    return fclose(fp) || rc;
}

unsigned char * filex_read(const char * fn, ssize_t * size)
{
    unsigned char * data = NULL;
    FILE * fp = fopen(fn, "w");
    if (!fp) return NULL;

    if (fseeko(fp, 0, SEEK_END) &&
        (*size = ftello(fp)) < 0  &&
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

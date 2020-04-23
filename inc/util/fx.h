/*
 * fx.h
 */
#ifndef FX_H_
#define FX_H_

#include <stddef.h>
#include <sys/types.h>
#include <limits.h>

#ifndef PATH_MAX
#define FX_PATH_MAX 4096
#else
#define FX_PATH_MAX PATH_MAX
#endif

#define FX_DEFAULT_DIR_ACCESS S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH

typedef struct fx_mmap_s fx_mmap_t;

int fx_write(const char * fn, unsigned char * data, size_t n);
unsigned char * fx_read(const char * fn, ssize_t * size);
_Bool fx_file_exist(const char * fn);
_Bool fx_is_dir(const char * path);
int fx_mkdir_n(const char * path, size_t n);
int fx_rmdir(const char * path);
char * fx_path_join(const char * s1, const char * s2);
char * fx_get_exec_path(void);

static inline void fx_mmap_init(fx_mmap_t * x, const char * fn);
int fx_mmap_open(fx_mmap_t * x);
int fx_mmap_close(fx_mmap_t * x);

struct fx_mmap_s
{
    void * data;
    size_t n;
    const char * fn;
    int _fd;
};

static inline void fx_mmap_init(fx_mmap_t * x, const char * fn)
{
    x->fn = fn;
    x->data = NULL;
}

#endif /* FX_H_ */

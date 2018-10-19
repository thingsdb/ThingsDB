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

int fx_write(const char * fn, unsigned char * data, size_t n);
unsigned char * fx_read(const char * fn, ssize_t * size);
_Bool fx_file_exist(const char * fn);
_Bool fx_is_dir(const char * path);
int fx_rmdir(const char * path);
char * fx_path_join(const char * s1, const char * s2);
char * fx_get_exec_path(void);

#endif /* FX_H_ */

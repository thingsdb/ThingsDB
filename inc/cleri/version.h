/*
 * version.h - cleri version information.
 */
#ifndef CLERI_VERSION_H_
#define CLERI_VERSION_H_

#define CLERI_VERSION_MAJOR 1
#define CLERI_VERSION_MINOR 0
#define CLERI_VERSION_PATCH 2

#define VERSION__STRINGIFY(num) #num
#define VERSION___STR(major,minor,patch) \
        VERSION__STRINGIFY(major) "." \
        VERSION__STRINGIFY(minor) "." \
        VERSION__STRINGIFY(patch)
/* end helpers */

/* start auto generated from above */
#define LIBCLERI_VERSION VERSION___STR( \
        CLERI_VERSION_MAJOR, \
        CLERI_VERSION_MINOR, \
        CLERI_VERSION_PATCH)

/* public funtion */
#ifdef __cplusplus
extern "C" {
#endif

const char * cleri_version(void);

#ifdef __cplusplus
}
#endif

#endif /* CLERI_VERSION_H_ */

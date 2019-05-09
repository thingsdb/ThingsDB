/*
 * version.h
 */
#ifndef TI_VERSION_H_
#define TI_VERSION_H_

#define TI_VERSION_MAJOR 0
#define TI_VERSION_MINOR 1
#define TI_VERSION_PATCH 2

/*
 * Use TI_VERSION_PRE_RELEASE for alpha release versions.
 * This should be an empty string when building a final release.
 * Examples: "-alpha-0" "-alpha-1", ""
 */
#define TI_VERSION_PRE_RELEASE "-alpha0"

#define TI_STRINGIFY(num) #num
#define TI_VERSION_STR(major,minor,patch) \
    TI_STRINGIFY(major) "." \
    TI_STRINGIFY(minor) "." \
    TI_STRINGIFY(patch)

#ifndef NDEBUG
#define TI_VERSION_BUILD_RELEASE "+debug"
#else
#define TI_VERSION_BUILD_RELEASE ""
#endif

#define TI_VERSION TI_VERSION_STR( \
        TI_VERSION_MAJOR, \
        TI_VERSION_MINOR, \
        TI_VERSION_PATCH) \
        TI_VERSION_PRE_RELEASE \
        TI_VERSION_BUILD_RELEASE

#define TI_MAINTAINER \
    "Jeroen van der Heijden <jeroen@transceptor.technology>"
#define TI_HOME_PAGE \
    "https://thingsdb.net"

int ti_version_cmp(const char * version_a, const char * version_b);
void ti_version_print(void);

/* ThingsDB can only connect with servers having at least this version. */
#define TI_MINIMAL_VERSION "0.1.0"


#endif /* TI_VERSION_H_ */

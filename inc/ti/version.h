/*
 * version.h
 */
#ifndef TI_VERSION_H_
#define TI_VERSION_H_

#define THINGSDB_VERSION_MAJOR 0
#define THINGSDB_VERSION_MINOR 0
#define THINGSDB_VERSION_PATCH 1

/*
 * Use THINGSDB_VERSION_PRE_RELEASE for alpha release versions.
 * This should be an empty string when building a final release.
 * Examples: "-alpha-0" "-alpha-1", ""
 */
#define THINGSDB_VERSION_PRE_RELEASE "-alpha0"

#define THINGSDB_STRINGIFY(num) #num
#define THINGSDB_VERSION_STR(major,minor,patch) \
    THINGSDB_STRINGIFY(major) "." \
    THINGSDB_STRINGIFY(minor) "." \
    THINGSDB_STRINGIFY(patch)

#ifndef NDEBUG
#define THINGSDB_VERSION_BUILD_RELEASE "+debug"
#else
#define THINGSDB_VERSION_BUILD_RELEASE ""
#endif

#define THINGSDB_VERSION THINGSDB_VERSION_STR( \
        THINGSDB_VERSION_MAJOR, \
        THINGSDB_VERSION_MINOR, \
        THINGSDB_VERSION_PATCH) \
        THINGSDB_VERSION_PRE_RELEASE \
        THINGSDB_VERSION_BUILD_RELEASE

#define THINGSDB_MAINTAINER \
    "Jeroen van der Heijden <jeroen@transceptor.technology>"
#define THINGSDB_HOME_PAGE \
    "https://thingsdb.net"

int ti_version_cmp(const char * version_a, const char * version_b);
void ti_version_print(void);

/* ThingsDB can only connect with servers having at least this version. */
#define THINGSDB_MINIMAL_VERSION "0.0.0"


#endif /* TI_VERSION_H_ */

/*
 * version.h
 */
#ifndef TI_VERSION_H_
#define TI_VERSION_H_

#define TI_VERSION_MAJOR 0
#define TI_VERSION_MINOR 6
#define TI_VERSION_PATCH 13

/* The syntax version is used to test compatibility with functions
 * using the `ti_nodes_check_syntax()` function */
#define TI_VERSION_SYNTAX 0

/* ThingsDB can only connect with servers having at least this version. */
#define TI_MINIMAL_VERSION "0.4.2"

/*
 * Use TI_VERSION_PRE_RELEASE for alpha release versions.
 * This should be an empty string when building a final release.
 * Examples:
 *  "-alpha-0"
 *  "-alpha-1"
 *  ""
 */
#define TI_VERSION_PRE_RELEASE ""

#define TI_MAINTAINER \
    "Jeroen van der Heijden <jeroen@transceptor.technology>"
#define TI_HOME_PAGE \
    "https://thingsdb.net"

/* start helpers */
#define VERSION__STRINGIFY(num) #num
#define VERSION___STR(major,minor,patch) \
        VERSION__STRINGIFY(major) "." \
        VERSION__STRINGIFY(minor) "." \
        VERSION__STRINGIFY(patch)
#define VERSION__SYNTAX_STR(v) "v" VERSION__STRINGIFY(v)
/* end helpers */

/* start auto generated from above */
#ifndef NDEBUG
#define TI_VERSION_BUILD_RELEASE "+debug"
#else
#define TI_VERSION_BUILD_RELEASE ""
#endif
#define TI_VERSION VERSION___STR( \
        TI_VERSION_MAJOR, \
        TI_VERSION_MINOR, \
        TI_VERSION_PATCH) \
        TI_VERSION_PRE_RELEASE \
        TI_VERSION_BUILD_RELEASE
#define TI_VERSION_SYNTAX_STR VERSION__SYNTAX_STR(TI_VERSION_SYNTAX)
/* end auto generated */

int ti_version_cmp(const char * version_a, const char * version_b);
void ti_version_print(void);



#endif /* TI_VERSION_H_ */

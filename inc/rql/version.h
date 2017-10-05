/*
 * version.h
 *
 *  Created on: Sep 29, 2017
 *      Author: Jeroen van der Heijden <jeroen@transceptor.technology>
 */
#ifndef RQL_VERSION_H_
#define RQL_VERSION_H_

#define RQL_VERSION_MAJOR 0
#define RQL_VERSION_MINOR 0
#define RQL_VERSION_PATCH 1

#define RQL_STRINGIFY(num) #num
#define RQL_VERSION_STR(major,minor,patch) \
    RQL_STRINGIFY(major) "." \
    RQL_STRINGIFY(minor) "." \
    RQL_STRINGIFY(patch)

#define RQL_VERSION RQL_VERSION_STR(                \
        RQL_VERSION_MAJOR,         \
        RQL_VERSION_MINOR,         \
        RQL_VERSION_PATCH)

#define RQL_BUILD_DATE __DATE__ " " __TIME__
#define RQL_MAINTAINER "Jeroen van der Heijden <jeroen@transceptor.technology>"
#define RQL_HOME_PAGE "http://rql.net"

int rql_version_cmp(const char * version_a, const char * version_b);

/* RQL can only connect with servers having at least this version. */
#define RQL_MINIMAL_VERSION "0.0.0"


#endif /* RQL_VERSION_H_ */

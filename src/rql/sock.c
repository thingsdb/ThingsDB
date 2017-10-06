/*
 * sock.c
 *
 *  Created on: Oct 5, 2017
 *      Author: Jeroen van der Heijden <jeroen@transceptor.technology>
 */
#include <sys/socket.h>
#include <rql/sock.h>


const char * rql_sock_ip_support_str(uint8_t ip_support)
{
    switch (ip_support)
    {
    case AF_UNSPEC: return "ALL";
    case AF_INET: return "IPV4ONLY";
    case AF_INET6: return "IPV6ONLY";
    default: return "UNKNOWN";
    }
}

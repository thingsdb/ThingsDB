/*
 * tcp.c
 */
#include <stdlib.h>
#include <string.h>
#include <ti/tcp.h>

static const size_t tcp__name_buf_sz = 54;

const char * ti_tcp_ip_support_str(int ip_support)
{
    switch (ip_support)
    {
    case AF_UNSPEC:
        return "ALL";
    case AF_INET:
        return "IPV4ONLY";
    case AF_INET6:
        return "IPV6ONLY";
    default:
        return "UNKNOWN";
    }
}

/*
 * Return a name for the connection if successful or NULL in case of a failure.
 *
 * The returned value is malloced and should be freed.
 */
char * ti_tcp_name(const char * prefix, uv_tcp_t * client)
{
    size_t n = strlen(prefix);
    char * buffer = malloc(tcp__name_buf_sz + n);
    struct sockaddr_storage name;
    int len = sizeof(name);     /* len is used both for input and output */

    if (!buffer || uv_tcp_getpeername(client, (struct sockaddr *) &name, &len))
        goto failed;

    memcpy(buffer, prefix, n);

    switch (name.ss_family)
    {
    case AF_INET:
        {
            char addr[INET_ADDRSTRLEN];
            uv_inet_ntop(
                    AF_INET,
                    &((struct sockaddr_in *) &name)->sin_addr,
                    addr,
                    sizeof(addr));
            snprintf(
                    buffer + n,
                    tcp__name_buf_sz,
                    "%s:%d",
                    addr,
                    ntohs(((struct sockaddr_in *) &name)->sin_port));
        }
        break;

    case AF_INET6:
        {
            char addr[INET6_ADDRSTRLEN];
            uv_inet_ntop(
                    AF_INET6,
                    &((struct sockaddr_in6 *) &name)->sin6_addr,
                    addr,
                    sizeof(addr));
            snprintf(
                    buffer + n,
                    tcp__name_buf_sz,
                    "[%s]:%d",
                    addr,
                    ntohs(((struct sockaddr_in6 *) &name)->sin6_port));
        }
        break;

    default:
        goto failed;
    }

    return buffer;

failed:
    free(buffer);
    return NULL;
}


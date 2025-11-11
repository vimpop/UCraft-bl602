#ifndef WRAPPER_CUSTOM_H
#define WRAPPER_CUSTOM_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <lwip/sockets.h>
#include <unistd.h>
#include <sys/select.h>
#include <signal.h>
#include <lwip/netdb.h>
#include <lwip/errno.h>
#include <FreeRTOS.h>
#include <task.h>

#include "tlsf.h"
#include "mbedtls/platform.h"
#include "config.h"
tlsf_t tlsf_memory_region;
// Endian swapping definitions
#define __bswap_16(x) ((u16_t)((((x) >> 8) & 0xff) | (((x) & 0xff) << 8)))
#define __bswap_32(x) ((u32_t)((((x) >> 24) & 0xff) |  \
                               (((x) >> 8) & 0xff00) | \
                               (((x) & 0xff00) << 8) | \
                               (((x) & 0xff) << 24)))
#define __bswap_64(x) ((u64_t)((((x) >> 56) & 0xff) |      \
                               (((x) >> 40) & 0xff00) |    \
                               (((x) >> 24) & 0xff0000) |  \
                               (((x) >> 8) & 0xff000000) | \
                               (((x) & 0xff000000) << 8) | \
                               (((x) & 0xff0000) << 24) |  \
                               (((x) & 0xff00) << 40) |    \
                               (((x) & 0xff) << 56)))

// Misc functions
static inline void U_wrapperEnd()
{
    return;
}
static inline void U_sleep(int msec)
{
    vTaskDelay(msec / portTICK_PERIOD_MS);
}
static inline uint64_t U_millis()
{
    return 0;
}
// Networking functions
static inline int U_socket(int domain, int type, int protocol)
{
    return lwip_socket(domain, type, protocol);
}

static inline int U_setsockopt(int sockfd, int level, int optname, const void *optval, socklen_t optlen)
{
    return lwip_setsockopt(sockfd, level, optname, optval, optlen);
}
static inline int U_setsocknonblock(int sockfd)
{
    return fcntl(sockfd, F_SETFL, O_NONBLOCK);
}
static inline int U_bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen)
{
    return lwip_bind(sockfd, addr, addrlen);
}

static inline int U_listen(int sockfd, int backlog)
{
    return lwip_listen(sockfd, backlog);
}

static inline int U_select(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout)
{
    return lwip_select(nfds, readfds, writefds, exceptfds, timeout);
}

static inline int U_accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen)
{
    return lwip_accept(sockfd, addr, addrlen);
}

static inline int U_getpeername(int sockfd, struct sockaddr *addr, socklen_t *addrlen)
{
    return lwip_getpeername(sockfd, addr, addrlen);
}

static inline struct hostent *U_gethostbyname(const char *name)
{
    return lwip_gethostbyname(name);
}

static inline char *U_inet_ntoa(struct in_addr in)
{
    return inet_ntoa(in);
}

static inline ssize_t U_read(int fd, void *buf, size_t count)
{
    return lwip_read(fd, buf, count);
}
static inline ssize_t U_recv(int sockfd, void *buf, size_t len, int flags)
{
    return lwip_recv(sockfd, buf, len, flags);
}

static inline ssize_t U_send(int sockfd, const void *buf, size_t len, int flags)
{
    ssize_t status = lwip_send(sockfd, buf, len, flags);
    return status;
}

static inline int U_connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen)
{
    return lwip_connect(sockfd, addr, addrlen);
}

static inline int U_close(int fd)
{
    return lwip_close(fd);
}

static inline int U_shutdown(int sockfd, int how)
{
    return lwip_shutdown(sockfd, how);
}
// Memory function
static inline void *U_malloc(size_t size)
{
    if (size == 0)
    {
        printf("malloc(0) is not allowed\n");
        return NULL;
    }
    return tlsf_malloc(tlsf_memory_region, size);
}

static inline void *U_calloc(size_t nmemb, size_t size)
{
    // use malloc
    void *p;
    p = tlsf_malloc(tlsf_memory_region, nmemb * size);
    if (p)
    {
        memset(p, 0, nmemb * size);
        return p;
    }
    return NULL;
}

static inline void *U_realloc(void *ptr, size_t size)
{
    return tlsf_realloc(tlsf_memory_region, ptr, size);
}

static inline void U_free(void *ptr)
{
    tlsf_free(tlsf_memory_region, ptr);
    return;
}

static inline void U_wrapperStart()
{
    unsigned char *buffer = pvPortMalloc(1024 * 77);
    tlsf_memory_region = tlsf_create_with_pool(buffer, 1024 * 77);
#ifdef ONLINE_MODE
    mbedtls_platform_set_calloc_free(U_calloc, U_free);
#endif /*ONLINE_MODE*/
    return;
}
#endif

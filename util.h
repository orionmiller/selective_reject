#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>

#include "cpe464.h"

#ifndef _UTIL_H
#define _UTIL_H

int s_socket(int domain, int type, int protocol);

ssize_t s_sendto(int socket, const void *message, size_t length, int flags, const struct sockaddr *dest_addr, socklen_t dest_len);

ssize_t s_recvfrom(int socket, void *buffer, size_t length, int flags, struct sockaddr *address, socklen_t *address_len);

void s_memcpy(void *dest, const void *src, size_t n);

void *s_malloc(size_t size);

void s_bind(int socket, const struct sockaddr *address, socklen_t address_len);

void s_getsockname(int socket, struct sockaddr *address, socklen_t *address_len);

struct hostent *s_gethostbyname(const char *name);

void s_listen(int socket, int backlog);

pid_t s_fork(void);


#endif

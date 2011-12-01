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
#include "util.h"

int s_socket(int domain, int type, int protocol)
{
  int sock_d;
  if ((sock_d= socket(domain, type, protocol)) < 0) //magic num
    {
      perror("s_socket");
      exit(EXIT_FAILURE);
    }
  return sock_d;
}

/* ssize_t s_sendto(int socket, const void *message, size_t length, int flags, const struct sockaddr *dest_addr, socklen_t dest_len) */
/* { */
/*   ssize_t ret_val; */
/*   ret_val = sendto(socket, message, length, flags, dest_addr, dest_len); */
/*   if (ret_val == -1 || ret_val != length) */
/*     { */
/*       perror("s_sendto"); */
/*       exit(EXIT_FAILURE); */
/*     } */
/*   return ret_val; */
/* } */

ssize_t s_recvfrom(int socket, void *buffer, size_t length, int flags, struct sockaddr *address, socklen_t *address_len)
{
  ssize_t ret_val;
  ret_val = recvfrom(socket, buffer, length, flags, address, address_len);
  if (ret_val < 0)
    {
      perror("s_recvfrom");
      exit(EXIT_FAILURE);
    }
  return ret_val;
}


void s_memcpy(void *dest, const void *src, size_t n)
{
  if (memcpy(dest,src,n) != dest)
    {
      fprintf(stderr, "%s\n","s_memcpy");
      exit(EXIT_FAILURE);
    }
}

void *s_malloc(size_t size)
{
  void *data;
  if((data=malloc(size)) == NULL)
    {
      perror("s_malloc");
      exit(EXIT_FAILURE);
    }
  return data;
}


void s_bind(int socket, const struct sockaddr *address, socklen_t address_len)
{
  if(bindMod(socket, address, address_len) < 0)
    {
      perror("s_bind");
      exit(EXIT_FAILURE);
    }
}


void s_getsockname(int socket, struct sockaddr *address, socklen_t *address_len)
{
  if (getsockname(socket, address, address_len) < 0)
    {
      perror("s_getsockbyname");
      exit(EXIT_FAILURE);
    }
}


struct hostent* s_gethostbyname(const char *name)
{
  struct hostent *hp;
  if((hp = gethostbyname(name))==NULL)
    {
      perror("s_gethostbyname");
      exit(EXIT_FAILURE);
    }
  return hp;
}

pid_t s_fork(void)
{
  pid_t pid;
  if ((pid = fork()) < 0)
    {
      perror("s_fork");
      exit(EXIT_FAILURE);
    }
  return pid;
}


size_t s_fread(void *ptr, size_t size, size_t nmemb, FILE *stream)
{
  size_t num_read  = fread(ptr, size, nmemb, stream);
  if(ferror(stream))
    {
      perror("s_fread");
      exit(EXIT_FAILURE);
    }
  return num_read;
}

size_t s_fwrite(const void *ptr, size_t size, size_t nmemb,
	      FILE *stream)
{
  size_t num_read = fwrite(ptr, size, nmemb, stream);
  if(ferror(stream))
    {
      perror("s_fread");
      exit(EXIT_FAILURE);
    }
  return num_read;
}

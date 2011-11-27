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
#include <sys/time.h>
#include <string.h>

#include "util.h"
#include "cpe464.h"
#include "libsrj.h"

sock *client_sock(char *remote_address, uint16_t remote_port, 
		  uint32_t buffsize, uint32_t window_size)
{  
  struct hostent *hp;
  sock *Client = s_malloc(sizeof(sock));

  Client->sock = s_socket(SOCK_DOMAIN, SOCK_TYPE, DEFAULT_PROTOCOL);
  Client->buffsize = buffsize;
  Client->window_size = window_size;

  //Remote Socket Address Set Up
  Client->remote.sin_family = SOCK_DOMAIN;
  hp = s_gethostbyname(remote_address);
  s_memcpy(&(Client->remote.sin_addr), hp->h_addr, hp->h_length);
  Client->remote.sin_port = htons(remote_port);

  Client->seq = 0;

  return Client;
}


sock *server_sock(uint32_t buffsize)
{  
  sock *Server = s_malloc(sizeof(sock));
  int len = sizeof(Server->local);
  Server->buffsize = buffsize;
  
  Server->sock = s_socket(SOCK_DOMAIN, SOCK_TYPE, DEFAULT_PROTOCOL);

  Server->local.sin_family = SOCK_DOMAIN;
  Server->local.sin_addr.s_addr = htonl(INADDR_ANY); //???
  Server->local.sin_port = htons(DEFAULT_PORT);

  s_bind(Server->sock, (struct sockaddr *)&(Server->local), 
  	 sizeof(Server->local));

  s_getsockname(Server->sock, (struct sockaddr *)&(Server->local), 
		(socklen_t *)&len);

  Server->seq = 0;
  return Server;
}

void send_pkt(pkt *Pkt, int socket, struct sockaddr_in dst_addr)
{
  int flags = 0;
  int dst_addr_len = (int)sizeof(dst_addr);
  if (Pkt != NULL && Pkt->datagram != NULL)
    {
      print_flag(Pkt->Hdr->flag);
      printf(" Seq: %u ;;\n", Pkt->Hdr->seq);
      sendtoErr(socket, Pkt->datagram, Pkt->datagram_len, flags, 
		(struct sockaddr *)&dst_addr, dst_addr_len);

    }

  else 
    fprintf(stderr, "send_pkt argument error\n");
}


int recv_pkt(pkt *Pkt, int socket, struct sockaddr_in *src_addr, uint32_t buffsize)
{
  int flags = 0;
  int src_addr_len = sizeof(*src_addr);

  if (Pkt != NULL && Pkt->datagram != NULL)
    {
      Pkt->datagram_len = s_recvfrom(socket, Pkt->datagram, buffsize+HDR_LEN, flags, 
       (struct sockaddr *)src_addr, (socklen_t *)&(src_addr_len));
      Pkt->data_len = Pkt->datagram_len - HDR_LEN;
      if (pkt_checksum(Pkt) == CHECKSUM_GOOD)
	{
	  get_hdr(Pkt);
	  return 1;
	}
      else
	return 0;
    }
  return 0;
}

void print_flag(uint8_t flag)
{
  printf("Flag: ");
  switch (flag)
    {
    case DATA:
      printf("DATA\n");
      break;

    case DATA_RESENT:
      printf("DATA_RESENT\n");
      break;

    case ACK:
      printf("ACK\n");
      break;

    case SREJ:
      printf("SREJ\n");
      break;

    case RR:
      printf("RR\n");
      break;

    case FILE_NAME:
      printf("FILE_NAME\n");
      break;

    case FILE_NAME_ACK:
      printf("FILE_NAME_ACK\n");
      break;

    case FILE_EOF:
      printf("FILE_EOF\n");
      break;

    case FILE_EOF_ACK:
      printf("FILE_EOF_ACK\n");
      break;
    }
  fflush(stdout);
}


void create_pkt(pkt *Pkt, uint8_t flag, uint32_t seq, uint8_t *data, uint32_t data_len)
{
  if (Pkt != NULL && ((data == NULL && data_len == 0)||(data != NULL && data_len > 0)))//MAGICNUM
    {
      Pkt->data_len = data_len;
      Pkt->datagram_len = HDR_LEN + data_len;
      memset(Pkt->datagram, 0,Pkt->datagram_len);
      if (data != NULL)
	s_memcpy(Pkt->data, data, Pkt->data_len);
      create_hdr(Pkt, seq, flag);
      get_hdr(Pkt);
    }
  else 
    {
      fprintf(stderr, "creat_pkt invalid arguments\n");
    }
}


void create_hdr(pkt *Pkt, uint32_t seq, uint8_t flag)
{
  uint16_t n_checksum = 0;
  uint32_t n_seq = htonl(seq);
  if (Pkt != NULL)
    {
      memset(Pkt->datagram, 0, HDR_LEN);
      s_memcpy(Pkt->datagram + HDR_SEQ_OFFSET, &n_seq, sizeof(uint32_t));
      s_memcpy(Pkt->datagram + HDR_FLAG_OFFSET, &flag, sizeof(uint16_t));
      n_checksum = in_cksum((unsigned short *)Pkt->datagram, Pkt->datagram_len);
      s_memcpy(Pkt->datagram + HDR_CHECKSUM_OFFSET, &n_checksum, sizeof(uint16_t));
    }
  else 
    {
      fprintf(stderr, "create_hdr Pkt is NULL\n");
    }
}


void get_hdr(pkt *Pkt)
{
  if (Pkt != NULL && Pkt->Hdr != NULL && Pkt->datagram != NULL)
    {
      s_memcpy(&(Pkt->Hdr->checksum), Pkt->datagram + HDR_CHECKSUM_OFFSET, sizeof(uint16_t));
      s_memcpy(&(Pkt->Hdr->flag), Pkt->datagram + HDR_FLAG_OFFSET,  sizeof(uint16_t));
      Pkt->Hdr->flag =  ntohs(Pkt->Hdr->flag);
      s_memcpy(&(Pkt->Hdr->seq), Pkt->datagram + HDR_SEQ_OFFSET, sizeof(uint32_t));
      Pkt->Hdr->seq = ntohl(Pkt->Hdr->seq);
    }
}


uint16_t pkt_checksum(pkt *Pkt)
{
  uint16_t checksum = 0;
  if (Pkt != NULL && Pkt->datagram != NULL)
    {
      checksum = in_cksum((unsigned short *)Pkt->datagram, Pkt->datagram_len);
    }
  //  else
  //    {
  //      fprintf(stderr, "pkt_checksum - bad Pkt\n");
  //      exit(EXIT_FAILURE);
  //    }
  return checksum;
}


void sendtoErr_setup(double error_rate)
{
  sendtoErr_init(error_rate, DROP_ON, FLIP_ON, DEBUG_ON, RSEED_ON);
}


pkt *pkt_alloc(uint32_t buffsize)
{
  pkt *Pkt = s_malloc(sizeof(pkt));
  Pkt->Hdr = s_malloc(sizeof(hdr));
  Pkt->datagram = s_malloc(HDR_LEN + buffsize);
  Pkt->data = Pkt->datagram + HDR_LEN;
  return Pkt;
}


int select_call(int socket, int seconds, int useconds)
{
  static struct timeval timeout;
  fd_set fdvar;
  int select_out;
  timeout.tv_sec = seconds;
  timeout.tv_usec = useconds;
  FD_ZERO(&fdvar);
  FD_SET(socket, &fdvar);
  select_out = selectMod(socket+1, (fd_set *)&fdvar, (fd_set *)0, (fd_set *)0, &timeout);
  if (select_out == -1) //magic num
    {
      perror("selectMod");
      exit(EXIT_FAILURE);
    }
  return select_out;//(FD_ISSET(socket, &fdvar));
}
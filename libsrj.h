// Author: Orion Miller (aomiller)
// 

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


#ifndef _LIBSRJ_H
#define _LIBSRJ_H


// [[ HDR ]]
//
// 0                                                           31
// |-----------------------------------------------------------|
// |                        Sequence                           |
// |-----------------------------------------------------------|
// 32                            47       54
// |-----------------------------------------------------------|
// |          Checksum           |  Flag   |      Data::       |
// |-----------------------------------------------------------|
//
// Sequence - (32 bits)
//   Sequence number of the packet.
//
// Flag - (8 bits)
//   Specific type of packet bein sent.
//
// Checksum - (16 bits)
//   The checksum of the Header and the Data.
//

typedef struct {
  uint32_t seq;
  uint16_t checksum;
  uint8_t flag;
} hdr;

#define HDR_LEN (sizeof(hdr))
#define HDR_SEQ_OFFSET (0)
#define HDR_CHECKSUM_OFFSET (4)
#define HDR_FLAG_OFFSET (6)


// [[ Flags ]]
//
// DATA - [ Regular Data ]
//  To be used during the data transfer.
//
// DATA_RESENT - []

// Flag & Value.
#define DATA (1)
#define DATA_RESENT (2)
#define ACK (3)
#define SREJ (4)
#define RR (5)
#define FILE_NAME (6)
#define FILE_NAME_ACK (7)
#define FILE_NAME_ERR_ACK (10)
#define FILE_EOF (8)
#define FILE_EOF_ACK (9)


// [[ Sockets ]]

typedef struct {
  int sock;
  struct sockaddr_in remote;
  struct sockaddr_in local;
  uint32_t seq;
  uint32_t buffsize;
  uint32_t window_size;
  uint32_t num_wait;
} sock;

//--Socket Set Up--//
#define SOCK_DOMAIN AF_INET
#define SOCK_TYPE SOCK_DGRAM
#define DEFAULT_PROTOCOL (0)
#define DEFAULT_PORT (0)



#define TRUE (1)
#define FALSE (0)


//--Packet--//

#define CHECKSUM_GOOD (0)

typedef struct {
  hdr *Hdr;
  uint8_t *datagram;
  uint32_t datagram_len;
  uint8_t *data;
  uint32_t data_len;
} pkt;


//--sendErr--//
#define ERROR_RATE_ZERO (0)

//--FLAG Specific Data Offsets--//
#define DATA_FILENAME_OFF (8)
#define DATA_BUFF_SIZE_OFF (0)
#define DATA_WINDOW_SIZE_OFF (4)
#define DATA_ERRNO_OFF (0)


//  [[ Prottypes ]]


//  - [ Set Up ]

// Client Sock
//   Generates and sets up a client socket to a remote conenction.
sock *client_sock(char *remote_address, uint16_t remote_port, 
		  uint32_t buffsize, uint32_t window_size);

// Server Sock
//   Generates and sets up a server socket.
sock *server_sock(uint32_t buffsize);

// Sets up the Send to Err
//  Pass in the error rate you want and it will call sendtoErr_init
//  with its according correct parameters.
void sendtoErr_setup(double error_rate);


// - [ Packets ]

// -- [ Allocate Packet ]
//
pkt *pkt_alloc(uint32_t buffsize);


// -- [ Create Packet ]
//
void create_pkt(pkt *Pkt, uint8_t flag, uint32_t seq, uint8_t *data, uint32_t data_len);


// -- [ Send Packet ]
//
void send_pkt(pkt *Pkt, int socket, struct sockaddr_in dst_addr);


// -- [ Receive Packet ]
//
int recv_pkt(pkt *Pkt, int socket, struct sockaddr_in *src_addr, uint32_t buffsize);


// -- [ Get Packet ]
//
void get_pkt(pkt *Pkt);


// -- [ Checksum ]
//
uint16_t pkt_checksum(pkt *Pkt);



// -- [ Header ]

// -- [ Create Header ]
//
void create_hdr(pkt *Pkt, uint32_t seq, uint8_t flag);


// -- [ Get Header ]
//
void get_hdr(pkt *Pkt);


// - [ General ]

// -- [ Select Call ]
//
int select_call(int socket, int seconds, int useconds);


// -- [ Print Flag ]
//
void print_flag(uint8_t flag);


#endif

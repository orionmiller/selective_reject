#include <stdio.h>
#include <stdlib.h>

#include "libsrj.h"
#include "util.h"

#ifndef _RCOPY_H
#define _RCOPY_H


#define MAX_RECV_WAIT_TIME_S (10) //seconds
#define MAX_RECV_WAIT_TIME_US (0) //micro-seconds

#define MAX_SEND_WAIT_TIME_S (1) //seconds
#define MAX_SEND_WAIT_TIME_US (0) //micro-seconds

#define MAX_TRIES (10)


#define ARGC_REMOTE_FILE (1)
#define ARGC_LOCAL_FILE (2)
#define ARGC_BUFFSIZE (3)
#define ARGC_ERROR_PERCENT (4)
#define ARGC_WINDOW_SIZE (5)
#define ARGC_REMOTE_MACHINE (6)
#define ARGC_REMOTE_PORT (7)
#define NUM_ARGS (8)

typedef struct {
  char *remote_file;
  char *local_file;
  uint32_t buffsize;
  double error_rate;
  uint32_t window_size;
  char *remote_machine;
  int32_t remote_port;
}arg;

typedef struct {
  char *name;
  FILE *fp;
  int err;
}file;

#define FILE_MODE "wb+"


enum State 
  { 
    S_START, S_FINISH, S_DONE,
    S_FILE_NAME, S_OPEN_FILE, S_GOOD_FILE, S_BAD_FILE, 
    S_FILE_TRANSFER, S_READ_FILE, S_ADJUST_WINDOW, S_SEND_WINDOW, S_TIMEOUT_ON_RESPONSE,
    S_FILE_EOF, 
    S_SEND, S_RECV, S_WAIT_ON_ACK, S_TIMEOUT_ON_ACK 
  };

typedef enum State STATE;


void process_arguments(arg *Arg, int argc, char *argv[]);
void process_server(sock *Client, arg *Arg);
STATE wait_on_ack(sock *Client, pkt *Pkt);
STATE timeout_on_ack(sock *Client);
STATE open_file(pkt *Pkt, file *File);
STATE good_file(file *File);
STATE bad_file(file *File);

void init_processing(sock *Client, file *LocalFile, file *RemoteFile, arg *Arg);
STATE send_init_pkt(sock *Client, file *File);

STATE create_file_eof_ack(sock *Client, pkt *SendPkt);
STATE file_name(sock *Client, file *LocalFile, file *RemoteFile);
STATE file_transfer(sock *Client, file *File);
STATE good_file(file *File);
STATE bad_file(file *File);
int get_file_err(pkt *Pkt);
STATE file_eof(sock *Client);
void create_init_pkt(sock *Client, pkt *Pkt, file *File);

#endif

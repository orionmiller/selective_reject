#include <stdio.h>
#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdio.h>

#include "libsrj.h"
#include "util.h"


#ifndef _SERVER_H
#define _SERVER_H

#define MAX_TRIES (10)

#define MAX_TRY_WAIT_TIME_S (1) //seconds
#define MAX_TRY_WAIT_TIME_US (0) //micro-seconds

#define MAX_RECV_WAIT_TIME_S (10) //seconds
#define MAX_RECV_WAIT_TIME_US (0) //micro-seconds

#define RECEIVED_PKT (1)

#define MAX_BUFF_SIZE (1400)

#define CHILD (0)
#define WAIT_ALL_CHILDREN (-1)

#define ERROR_PERCENT_ARGC (1)
#define NUM_ARGS (2)

#define ARG_ERROR_RATE (1)

typedef struct {
  char *name;
  FILE *fp;
  int err;
}file;

#define FILE_MODE "rb"


enum State 
  { 
    S_START, S_FINISH, S_DONE,
    S_FILE_NAME, S_OPEN_FILE, S_GOOD_FILE, S_BAD_FILE, 
    S_FILE_TRANSFER, S_READ_FILE, S_ADJUST_WINDOW, S_SEND_WINDOW, S_TIMEOUT_ON_RESPONSE,
    S_WAIT_ON_RESPONSE, S_FILE_EOF, 
    S_SEND, S_RECV, S_WAIT_ON_ACK, S_TIMEOUT_ON_ACK 
  };

typedef enum State STATE;

void process_arguments(double *error_rate, int argc, char *argv[]);
void wait_for_children(int *status);
void process_client(sock Old_Server, pkt InitPkt);
STATE open_file(file *File);
STATE good_file(sock *Server, pkt *SendPkt);
STATE bad_file(sock *Server, pkt *SendPkt, file *File);
STATE file_name(sock*Server, file *File);
STATE file_transfer(sock*Server, file *File);
STATE file_eof(sock *Server);
void transfer_setup(sock *Server, sock *Old_Server);
STATE wait_on_ack(sock *Server, pkt *RecvPkt);
STATE timeout_on_ack(int *num_wait);
STATE create_file_eof(sock *Server, pkt *SendPkt);
STATE process_init_syn(sock *Server, file *File, pkt *InitPkt);
char *get_filename(pkt *Pkt);
uint32_t get_buffsize(pkt *Pkt);
uint32_t get_window_size(pkt *Pkt);
#endif 

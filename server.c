#include <stdio.h>
#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdio.h>

#include "libsrj.h"
#include "util.h"

#include "server.h"

int main(int argc, char *argv[])
{
  pid_t pid;
  double error_rate;
  int status;
  pkt *RecvPkt = pkt_alloc(MAX_BUFF_SIZE);
  sock *Server = server_sock(MAX_BUFF_SIZE);

  printf("Port: %d\n", ntohs(Server->local.sin_port));
  process_arguments(&error_rate, argc, argv);
  sendtoErr_setup(error_rate);

  while (TRUE)
    {
      if(recv_pkt(RecvPkt, Server->sock, &(Server->local), Server->buffsize))
	{
	  pid = s_fork();
      
	  if (pid == CHILD)
	    {

	      process_client(Server, RecvPkt);
	      exit(EXIT_SUCCESS);
	    }
      
	  wait_for_children(&status);
	}
    }

  return EXIT_SUCCESS;
}


void process_arguments(double *error_rate, int argc, char *argv[])
{
  if (argc != NUM_ARGS)
    {
      fprintf(stderr, "usage: ./server <error-rate>\n");
      exit(EXIT_FAILURE);
    }
  *error_rate = atof(argv[ARG_ERROR_RATE]);
}


void wait_for_children(int *status)
{
  while(waitpid(WAIT_ALL_CHILDREN, status, WNOHANG) > 0)
    {
      printf("Processed\n");
    }
}


void process_client(sock *Old_Server, pkt *InitPkt) {
  STATE state = S_START;
  sock *Server = s_malloc(sizeof(sock));
  file *File = s_malloc(sizeof(file));

  transfer_setup(Server, Old_Server);

  while (state != S_FINISH) {
    switch (state) {
      
    case S_START:
      process_init_syn(Server, File, InitPkt);
      break;
      
    case S_FILE_NAME:
      state = file_name(Server, File);
      break;
      
    case S_FILE_TRANSFER:
      state = file_transfer(Server, File);
      break;
      
    case S_FILE_EOF:
      state = file_eof(Server);
      break;
      
    case S_RECV:
      break;
	
    default:
      printf("Process Client - Unkown Case\n");
      state= S_FINISH;
      break;
    }
  }
}


STATE file_name(sock*Server, file *File)
{
  STATE state = S_OPEN_FILE;
  pkt *SendPkt = pkt_alloc(Server->buffsize);

  while (state != S_DONE) 
    {
      switch (state)
	{
	case S_OPEN_FILE:
	  state = open_file(File);
	  break;
	  
	case S_GOOD_FILE:
	  state = good_file(Server, SendPkt);
	  break;
	  
	case S_BAD_FILE:
	  state = bad_file(Server, SendPkt, File);
	  break;

	case S_SEND:
	  state = S_DONE;
	  send_pkt(SendPkt, Server->sock, Server->remote);
	  break;

	default:
	  printf("File Name - Unkown Case\n");
	  return S_FINISH;
	}
    }
  return S_FILE_TRANSFER;  
}

STATE open_file(file *File)
{
  if (File !=NULL && File->name != NULL)
    {
      File->fp = fopen((const char *)File->name, FILE_MODE);
      if (File->fp)
	{
	  return S_GOOD_FILE;
	}
      File->err = errno;
      return S_BAD_FILE;
    }
  return S_DONE;
}


STATE good_file(sock *Server, pkt *SendPkt)
{
  create_pkt(SendPkt, FILE_NAME_ACK, Server->seq, NULL, 0);
  return S_SEND;
}


STATE bad_file(sock *Server, pkt *SendPkt, file *File)
{
  uint8_t *data = (uint8_t *)&(File->err);
  uint32_t data_len = sizeof(File->err);
  create_pkt(SendPkt, FILE_NAME_ERR_ACK, Server->seq, data, data_len);
  return S_SEND;
}


STATE file_transfer(sock*Server, file *File)
{
  STATE state = S_OPEN_FILE;

  while (state != S_DONE)
    {
      switch (state)
	{
	case S_READ_FILE:
	  break;
	  
	case S_ADJUST_WINDOW:
	  break;
	  
	case S_SEND_WINDOW:
	  break;

	case S_WAIT_ON_RESPONSE:
	  break;
	  
	case S_TIMEOUT_ON_RESPONSE:
	  break;

	default:
	  printf("File Transfer - Unkown Case\n");
	  return S_FINISH;
	}
    }
  return S_FILE_EOF;  
}



STATE file_eof(sock *Server)
{
  STATE state = S_OPEN_FILE;
  pkt *SendPkt = pkt_alloc(Server->buffsize);
  pkt *RecvPkt = pkt_alloc(Server->buffsize);
  int num_wait = 0;

  while (state != S_DONE) 
    {
      switch (state)
	{
	case S_FILE_EOF:
	  state = create_file_eof(Server, SendPkt);
	  break;

	case S_SEND:
	  send_pkt(SendPkt, Server->sock, Server->remote);
	  state = S_WAIT_ON_ACK;
	  break;

	case S_WAIT_ON_ACK:
	  state = wait_on_ack(Server, RecvPkt);
	  break;
	  
	case S_TIMEOUT_ON_ACK:
	  state = timeout_on_ack(&num_wait);
	  break;

	default:
	  printf("File EOF - Unkown Case\n");
	  return S_FINISH;
	}
    }
  return S_FINISH;  
}

STATE create_file_eof(sock *Server, pkt *SendPkt)
{
  static int bool = 1;
  if (bool) 
    {
      bool = 0;
      Server->seq++;
    }
  create_pkt(SendPkt, FILE_EOF, Server->seq, NULL, 0);
  return S_SEND;
}


STATE timeout_on_ack(int *num_wait)
{
  if (*num_wait >= 10)
    return S_DONE;
  
  *num_wait += 1;
  return S_SEND;
}

STATE wait_on_ack(sock *Server, pkt *RecvPkt)
{
  while (select_call(Server->sock, MAX_TRY_WAIT_TIME_S, MAX_TRY_WAIT_TIME_US))
    {
      recv_pkt(RecvPkt, Server->sock, &(Server->remote), Server->buffsize);
      if (RecvPkt->Hdr->seq == Server->seq)
	{
	  return S_DONE;
	}
    }
  return S_TIMEOUT_ON_ACK;
}

void transfer_setup(sock *Server, sock *Old_Server)
{
  *Server = *Old_Server;
  //getsock name stuff
}

STATE process_init_syn(sock *Server, file *File, pkt *InitPkt)
{
  Server->buffsize = get_buffsize(InitPkt);
  Server->window_size = get_window_size(InitPkt);
  File->name = get_filename(InitPkt);
  return S_FILE_NAME;
}

uint32_t get_buffsize(pkt *Pkt)
{
  uint32_t buffsize = MAX_BUFF_SIZE;
  s_memcpy(&buffsize, Pkt->data+DATA_BUFF_SIZE_OFF, sizeof(uint32_t));
  buffsize = ntohl(buffsize);
  return buffsize;
}

char *get_filename(pkt *Pkt)
{
  char *filename;
  uint32_t len = 0;
  len = strlen((char *)Pkt->data+DATA_FILENAME_OFF);
  filename = s_malloc((++len));
  s_memcpy(&filename, Pkt->data+DATA_FILENAME_OFF, len);
  return filename;
}

uint32_t get_window_size(pkt *Pkt)
{
  uint32_t window_size;
  s_memcpy(&window_size, Pkt->data+DATA_WINDOW_SIZE_OFF, sizeof(uint32_t));
  window_size = ntohl(window_size);
  return window_size;
}

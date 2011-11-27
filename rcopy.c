#include <stdio.h>
#include <stdlib.h>

#include "rcopy.h"
#include "libsrj.h"
#include "util.h"

int main(int argc, char *argv[])
{
  sock *Client;
  arg *Arg = s_malloc(sizeof(arg));

  process_arguments(Arg, argc, argv);
  Client = client_sock(Arg->remote_machine, Arg->remote_port, Arg->buffsize, Arg->window_size);
  sendtoErr_setup(Arg->error_rate);
  process_server(Client, Arg);
  return EXIT_SUCCESS;
}


void process_server(sock *Client, arg *Arg)
{
  STATE state = S_START;
  file *LocalFile = s_malloc(sizeof(file));
  file *RemoteFile = s_malloc(sizeof(file));

  LocalFile->name = Arg->local_file;
  RemoteFile->name = Arg->remote_file;
  

  while (state != S_FINISH) {
    switch (state) {
      
    case S_START:
      state = send_init_pkt(Client, RemoteFile);
      break;
      
    case S_FILE_NAME:
      state = file_name(Client, LocalFile, RemoteFile);
      break;
      
    case S_FILE_TRANSFER:
      state = file_transfer(Client, LocalFile);
      break;
      
    case S_FILE_EOF:
      state = file_eof(Client);
      break;
      
    default:
      printf("Process Client - Unkown Case\n");
      state= S_FINISH;
      break;
    }
  }
}

STATE file_transfer(sock *Client, file *File)
{
  return S_FILE_EOF;
}

STATE send_init_pkt(sock *Client, file *File)
{
  pkt *SendPkt = pkt_alloc(Client->buffsize);
  create_init_pkt(Client, SendPkt, File);
  send_pkt(SendPkt, Client->sock, Client->remote);
  free(SendPkt);
  return S_FILE_NAME;
}

void create_init_pkt(sock *Client, pkt *Pkt, file *File)
{
  uint8_t *data;
  uint32_t data_len;
  uint32_t filename_len = strlen(File->name) + 1;
  data_len = sizeof(uint32_t) + sizeof(uint32_t) + filename_len;
  data = s_malloc(data_len);
  s_memcpy(data + DATA_BUFF_SIZE_OFF, &(Client->buffsize), sizeof(uint32_t)); 
  s_memcpy(data + DATA_WINDOW_SIZE_OFF, &(Client->window_size), sizeof(uint32_t));
  s_memcpy(data + DATA_FILENAME_OFF, File->name, filename_len);
  create_pkt(Pkt, FILE_NAME, Client->seq, data, data_len);
}

STATE file_eof(sock *Client)
{
  STATE state = S_OPEN_FILE;
  pkt *SendPkt = pkt_alloc(Client->buffsize);

  while (state != S_DONE) 
    {
      switch (state)
	{
	case S_FILE_EOF:
	  state = create_file_eof_ack(Client, SendPkt);
	  break;

	case S_SEND:
	  send_pkt(SendPkt, Client->sock, Client->remote);
	  state = S_DONE;
	  break;

	default:
	  printf("File EOF - Unkown Case\n");
	  return S_FINISH;
	}
    }
  return S_FINISH;  
}

STATE create_file_eof_ack(sock *Client, pkt *SendPkt)
{
  create_pkt(SendPkt, FILE_EOF_ACK, Client->seq, NULL, 0); 
  return S_SEND;
}


STATE file_name(sock *Client, file *LocalFile, file *RemoteFile)
{
  STATE state = S_OPEN_FILE;
  pkt *RecvPkt = pkt_alloc(Client->buffsize);

  while (state != S_DONE) 
    {
      switch (state)
	{
	case S_OPEN_FILE:
	  state = open_file(RecvPkt, RemoteFile);
	  break;
	  
	case S_GOOD_FILE:
	  state = good_file(LocalFile);
	  break;
	  
	case S_BAD_FILE:
	  state = bad_file(RemoteFile);
	  break;

	case S_WAIT_ON_ACK:
	  state = wait_on_ack(Client, RecvPkt);
	  break;

	case S_TIMEOUT_ON_ACK:
	  return timeout_on_ack(Client);
	  break;

	case S_FINISH:
	  return S_FINISH;

	default:
	  printf("File Name - Unkown Case\n");
	  return S_FINISH;
	}
    }
  return S_FILE_TRANSFER;  
}

STATE wait_on_ack(sock *Client, pkt *Pkt)
{
  while (select_call(Client->sock, MAX_SEND_WAIT_TIME_S, MAX_SEND_WAIT_TIME_US))
    {
      recv_pkt(Pkt, Client->sock, &(Client->remote), Client->buffsize);
      if (Pkt->Hdr->seq == Client->seq)
	{
	  return S_OPEN_FILE;
	}
    }
  return S_TIMEOUT_ON_ACK;
}

STATE timeout_on_ack(sock *Client)
{
  if (Client->num_wait >= 10)
    return S_FINISH;
  
  Client->num_wait += 1;
  return S_START;
}

STATE open_file(pkt *Pkt, file *File)
{
  if (File != NULL && Pkt != NULL)
    {
      if (Pkt->Hdr->flag == FILE_NAME_ACK)
	{
	  return S_GOOD_FILE;
	}

      else if (Pkt->Hdr->flag == FILE_NAME_ERR_ACK)
	{
	  File->err = get_file_err(Pkt);
	  return S_BAD_FILE;
	}
    }
  return S_FINISH;
}

int get_file_err(pkt *Pkt)
{
  uint32_t err;
  s_memcpy(&err, Pkt->data+DATA_ERRNO_OFF, sizeof(uint32_t));
  err = ntohl(err);
  return (int)err;
}

STATE good_file(file *File)
{
  if (File != NULL && File->name != NULL)
    {
      if ((File->fp = fopen(File->name, FILE_MODE)) == NULL)
	{
	  perror("Opening Local File");
	  exit(EXIT_FAILURE);
	}
      return S_DONE;
    }
  return S_FINISH;
}

STATE bad_file(file *File)
{
  if (File != NULL)
    {
      fprintf(stderr, "Remote File Errr: %s", strerror(File->err));
    }
  return S_FINISH;
}

void process_arguments(arg *Arg, int argc, char *argv[])
{
  if (argc != NUM_ARGS)
    {
      printf("usage: ./rcopy <remote file> <local file> <buffsize> <error percent> <window size> <remote addr> <remote port>");
      exit(EXIT_FAILURE);
    }

  Arg->remote_file = argv[ARGC_REMOTE_FILE];
  Arg->local_file = argv[ARGC_LOCAL_FILE];
  Arg->buffsize = atoi(argv[ARGC_BUFFSIZE]);
  Arg->error_rate = atof(argv[ARGC_ERROR_PERCENT]);
  Arg->window_size = atoi(argv[ARGC_WINDOW_SIZE]);
  Arg->remote_machine = argv[ARGC_REMOTE_MACHINE];
  Arg->remote_port = atoi(argv[ARGC_REMOTE_PORT]);
}

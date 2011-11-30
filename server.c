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
      if(recv_pkt(RecvPkt, Server->sock, &(Server->remote), Server->buffsize))
	{
	  printf("Received Init Packet\n");
	  pid = s_fork();
     
	  if (pid == CHILD)
	    {

	      printf("Begin Processing of Client\n");
	      process_client(*Server, *RecvPkt);
	      exit(EXIT_SUCCESS);
	    }
      
	  printf("Wait for Children\n");
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


void process_client(sock Old_Server, pkt InitPkt) {
  STATE state = S_START;
  sock *Server = s_malloc(sizeof(sock));
  file *File = s_malloc(sizeof(file));

  transfer_setup(Server, &Old_Server);
  printf("Port: %d\n", ntohs(Server->local.sin_port));

  while (state != S_FINISH) {
    switch (state) {
      
    case S_START:
      printf("Start\n");
      state = process_init_syn(Server, File, &InitPkt);
      break;
      
    case S_FILE_NAME:
      printf("File Name\n");
      state = file_name(Server, File);
      break;
      
    case S_FILE_TRANSFER:
      printf("File Transfer\n");
      state = file_transfer(Server, File);
      break;
      
    case S_FILE_EOF:
      printf("File EOF\n");
      state = file_eof(Server);
      break;
	
    default:
      printf("Process Client - Unkown Case\n");
      state= S_FINISH;
      break;
    }
  }
  free(Server);
  free(File);
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
	  printf("Open File\n");
	  state = open_file(File);
	  break;
	  
	case S_GOOD_FILE:
	  printf("Good File\n");
	  state = good_file(Server, SendPkt);
	  break;
	  
	case S_BAD_FILE:
	  printf("Bad File\n");
	  state = bad_file(Server, SendPkt, File);
	  break;

	case S_SEND:
	  printf("Send\n");
	  state = S_DONE;
	  send_pkt(SendPkt, Server->sock, Server->remote);
	  break;

	default:
	  printf("File Name - Unkown Case\n");
	  return S_FINISH;
	}
    }
  free(SendPkt);
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
  uint32_t data  = File->err;
  uint32_t data_len = sizeof(File->err);
  data = htonl(data);
  create_pkt(SendPkt, FILE_NAME_ERR_ACK, Server->seq, (uint8_t *)&data, data_len);
  return S_SEND;
}


STATE file_transfer(sock*Server, file *File)
{
  STATE state = S_DONE;
  window *Window = window_alloc(Server);
  pkt *RecvPkt = pkt_alloc(Server->buffsize);
  // renitialize window attributes
  //   -- one attribute to initialize is num_wait = 0
  //special case for 1st window --- setup window & state = S_FILL_WINDOW
  while (state != S_DONE)
    {
      switch (state)
	{
	case S_ADJUST_WINDOW:
	  state = adjust_window(Window);
	  break;

	case S_FILL_WINDOW:
	  state = fill_window(Window, File);
	  break;
	  
	case S_SEND_WINDOW:
	  state = send_window(Server, Window);
	  break;

	case S_WAIT_ON_RESPONSE:
	  state = wait_on_response(Server, Window, RecvPkt);
	  break;
	  
	case S_TIMEOUT_ON_RESPONSE:
	  state = timeout_on_response(Window);
	  break;

	default:
	  printf("File Transfer - Unkown Case\n");
	  return S_FINISH;
	}
    }
  return S_FILE_EOF;  
}

STATE adjust_window(window *Window)
{
  uint32_t seq;
  if (Window->rr > Window->srej)
    seq = Window->rr - 1;
  else
    seq = Window->srej - 1;

  for (;seq >= Window->bottom; seq--)
      set_frame_empty(Window, seq);

  Window->bottom = seq + 1;
  Window->top = Window->bottom + Window->size - 1;

  return S_FILL_WINDOW;
}

STATE fill_window(window *Window, file *File)
{
  uint32_t seq = Window->bottom;

  if (Window->eof)
    return S_SEND_WINDOW;
  
  for (; seq <= Window->top; seq++)
    if (empty_frame(Window, get_frame_num(Window, seq)))
      break;

  for (; Window->eof == FALSE && seq <= Window->top; seq++)
    file_fill_frame(Window, File, seq);

  return S_SEND_WINDOW;
}

STATE send_window(sock *Server, window *Window)
{
  uint32_t seq = Window->bottom;
  for (; seq <= Window->top; seq++)
    if (full_frame(Window, get_frame_num(Window, seq)))
      {
	send_frame(Server, Window->Frame[get_frame_num(Window, seq)]);
	if (select_call(Server->sock, 0, 0))
	  return S_WAIT_ON_RESPONSE;
      }

  return S_WAIT_ON_RESPONSE;
}


STATE wait_on_response(sock *Server, window *Window, pkt *RecvPkt)
{
  if (select_call(Server->sock, MAX_WINDOW_WAIT_TIME_S, MAX_WINDOW_WAIT_TIME_US))
    {
      return S_FINISH;
      //return process_pkt(Window, RecvPkt);
    }
  // if (select call for a second)
  //  return process_pkt(RecvPkt);
  return S_WAIT_ON_RESPONSE;
}

//process_pkt RecvPkt
// if Pkt is RR (boundary checks?)
//  Window->rr = RR
// if Pkt is SREJ (currently going with no boundary checks for SREJ)
//  Window->srej = SREJ
//  set all packets as not-sent
// 
//  call adjust window
STATE timeout_on_response(window *Window)
{
  if (Window->num_wait < MAX_TRIES)
    {
      Window->num_wait += 1;
      return S_SEND_WINDOW;
    }
  return S_FINISH;
}


STATE file_eof(sock *Server)
{
  STATE state = S_FILE_EOF;
  pkt *SendPkt = pkt_alloc(Server->buffsize);
  pkt *RecvPkt = pkt_alloc(Server->buffsize);
  int num_wait = 0;
  Server->num_wait = 0;

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
  free(SendPkt);
  free(RecvPkt);
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
  uint32_t len = sizeof(Server->local);
  *Server = *Old_Server;
  s_getsockname(Server->sock, (struct sockaddr *)&(Server->local), &len);
  //getsock name stuff
}

STATE process_init_syn(sock *Server, file *File, pkt *InitPkt)
{
  printf("Process Init Pkt\n");
  Server->buffsize = get_buffsize(InitPkt);
  printf("Buffsize: %u\n", Server->buffsize);
  Server->window_size = get_window_size(InitPkt);
  printf("Window Size: %u\n", Server->window_size);
  File->name = get_filename(InitPkt);
  printf("File Name: %s\n", File->name);
  fflush(stdout);
  return S_FILE_NAME;
}

uint32_t get_buffsize(pkt *Pkt)
{
  uint32_t buffsize = MAX_BUFF_SIZE;
  printf("Getting Buffsize\n");
  s_memcpy(&buffsize, Pkt->data+DATA_BUFF_SIZE_OFF, sizeof(uint32_t));
  buffsize = ntohl(buffsize);
  return buffsize;
}

char *get_filename(pkt *Pkt)
{
  char *filename;
  uint32_t len = 0;
  uint32_t i = 0;
  printf("Getting Filename\n");
  len = strlen((char *)Pkt->data+DATA_FILENAME_OFF) + 1;
  printf("String Length %u\n", len);
  filename = (char *)s_malloc(len);
  printf("Malloced Filename\n");
  s_memcpy(filename, (char *)Pkt->data+DATA_FILENAME_OFF, len);
  printf("Memcpyed Filename\n");
  for (i = 0; i < len; i++)
    {
      printf("%d: %c", i, *(Pkt->data+DATA_FILENAME_OFF+i));
    }
  printf("Filename: \'%s\'\n", filename);
  return filename;
}

uint32_t get_window_size(pkt *Pkt)
{
  uint32_t window_size;
  printf("Getting Window Size\n");
  s_memcpy(&window_size, Pkt->data+DATA_WINDOW_SIZE_OFF, sizeof(uint32_t));
  window_size = ntohl(window_size);
  return window_size;
}

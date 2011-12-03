#include <stdio.h>
#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdio.h>

#include "libsrj.h"
#include "util.h"

#include "server.h"
#include <time.h>

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
	  if (init_pkt(RecvPkt))
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
    }
      free_pkt(RecvPkt);
      free(Server);

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
  pkt *RecvPkt = pkt_alloc(Server->buffsize);

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
	  state = S_FILE_BEGIN;
	  send_pkt(SendPkt, Server->sock, Server->remote);
	  break;

	case S_FILE_BEGIN:
	  printf("File Begin\n");
	  state = file_begin(Server, SendPkt, RecvPkt);
	  break;

	case S_FINISH:
	  printf("S_FINISH\n");
	  return S_FINISH;

	default:
	  printf("File Name - Unkown Case\n");
	  return S_FINISH;
	}
    }
  free_pkt(SendPkt);
  free_pkt(RecvPkt);
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
  STATE state = S_FILL_WINDOW;
  window *Window = window_alloc(Server);
  pkt *RecvPkt = pkt_alloc(Server->buffsize);
  Window->rr = 0;
  //  Window->bottom += 1;
  //  Window->top += 1;
  // renitialize window attributes
  //   -- one attribute to initialize is num_wait = 0
  //special case for 1st window --- setup window & state = S_FILL_WINDOW
  while (state != S_DONE)
    {
      switch (state)
	{
	case S_ADJUST_WINDOW:
	  printf("Adjust Window\n");
	  printf("Before Adjust\n");
	  //print_window(Window);  
	  state = adjust_window(Window);
	  break;

	case S_FILL_WINDOW:
	  printf("Fill Window\n");
	  state = fill_window(Window, File);
	  break;
	  
	case S_SEND_WINDOW:
	  printf("Send Window\n");
	  printf("Before Send\n");
	  //print_window(Window);  
	  state = send_window(Server, Window, RecvPkt);
	  printf("After Sent\n");
	  //print_window(Window);  
	  break;

	case S_WAIT_ON_RESPONSE:
	  printf("Wait On Response\n");
	  state = wait_on_response(Server, Window, RecvPkt);
	  break;
	  
	case S_TIMEOUT_ON_RESPONSE:
	  printf("Timeout On Response\n");
	  state = timeout_on_response(Window);
	  break;

	case S_FILE_NAME:
	  return S_FILE_NAME;

	case S_FINISH:
	  return S_FINISH;

	default:
	  printf("File Transfer - Unkown Case\n");
	  return S_FINISH;
	}
    }
  free_pkt(RecvPkt);
  free_window(Window);
  return S_FILE_EOF;  
}

STATE adjust_window(window *Window)
{
  //make sure funciton can break out if eof and RR is above highest sent pkt
  uint32_t seq;

  print_window(Window);
  if (Window->eof && Window->rr > Window->top_sent)
    return S_DONE;


  if (Window->rr > Window->bottom || Window->srej > Window->bottom)
    {
      seq = (Window->rr > Window->srej) ? (Window->rr -1) : (Window->srej -1);

      for (;seq >= Window->bottom; seq--)
	set_frame_empty(Window, get_frame_num(Window, seq));
      
      Window->bottom = (Window->rr > Window->srej) ? (Window->rr) : (Window->srej);
      Window->top = Window->bottom + Window->size - 1;
      
      return S_FILL_WINDOW;
    }

  return S_SEND_WINDOW;
}

STATE fill_window(window *Window, file *File)
{
  uint32_t seq = Window->bottom;

  printf("Fill Seq Start: %u\n", seq);
  for (; seq <= Window->top; seq++)
    if (empty_frame(Window, get_frame_num(Window, seq)))
      file_fill_frame(Window, File, seq);

  return S_SEND_WINDOW;
}

STATE send_window(sock *Server, window *Window, pkt *RecvPkt)
{
  uint32_t seq = Window->bottom;

  for (; seq <= Window->top; seq++)
    {
      if (full_frame(Window, get_frame_num(Window, seq)))
	{
	  send_frame(Server, Window->Frame[get_frame_num(Window, seq)]);
	  printf("Sent Frame: seq %u\n", seq);
	  if (seq > Window->top_sent)
	    Window->top_sent = seq;
	}
    }
  return S_WAIT_ON_RESPONSE;
}

STATE wait_on_response(sock *Server, window *Window, pkt *RecvPkt)
{
  if (select_call(Server->sock, MAX_WINDOW_WAIT_TIME_S, MAX_WINDOW_WAIT_TIME_US))
    if (recv_pkt(RecvPkt, Server->sock, &(Server->remote), Server->buffsize))
      {
	Window->num_wait = 0;
	printf("Wait - Proccess - Received RecvPkt\n");
	switch (RecvPkt->Hdr->flag)
	  {
	  case RR:
	    printf("Received an RR\n");
	    if (RecvPkt->Hdr->seq > Window->rr)
	      Window->rr = RecvPkt->Hdr->seq;
	    return S_ADJUST_WINDOW;
	    
	  case SREJ:
	    printf("Received a RR\n");
	    if (RecvPkt->Hdr->seq > Window->srej)
	      Window->srej = RecvPkt->Hdr->seq;
	    return S_ADJUST_WINDOW;
	    
	  case FILE_NAME:
	    return S_FILE_NAME;
	    break;
	    
	  default:
	    print_hdr(RecvPkt);
	    printf("File Transfer -- Unknown Packet\n");
	    //return S_ADJUST_WINDOW;
	    return S_FINISH;
	  }
      }

  return S_TIMEOUT_ON_RESPONSE;
}


void process_pkt(sock *Server, window *Window, pkt *Pkt)
{
  printf("Process Pkt\n");

  if(recv_pkt(Pkt, Server->sock, &(Server->remote), Server->buffsize))
    {
      printf("Received Pkt\n");
      switch (Pkt->Hdr->flag)
	{
	case RR:
	  printf("Received an RR\n");
	  if (Pkt->Hdr->seq > Window->rr)
	    Window->rr = Pkt->Hdr->seq;
	  //return S_ADJUST_WINDOW;
	  
	case SREJ:
	  printf("Received a RR\n");
	  if (Pkt->Hdr->seq > Window->srej)
	    Window->srej = Pkt->Hdr->seq;
	  //return S_ADJUST_WINDOW;
	  
	  /* case FILE_NAME: */
	  /*   return S_FILE_NAME; */
	  /*   break; */
	  
	default:
	  print_hdr(Pkt);
	  printf("File Transfer -- Unknown Packet\n");
	  //return S_FINISH;
	}
    }
}

STATE timeout_on_response(window *Window)
{
  uint32_t seq;
  printf("Window Num Wait: %u\n", Window->num_wait);
  if (Window->num_wait < MAX_TRIES)
    {
      Window->num_wait += 1;
      for (seq = Window->bottom; seq <= Window->top; seq++)
	if (sent_frame(Window, get_frame_num(Window, seq)))
	  set_frame_full(Window, get_frame_num(Window, seq));
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
  free_pkt(SendPkt);
  free_pkt(RecvPkt);
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
	  
STATE file_begin(sock *Server, pkt *SendPkt, pkt *RecvPkt)
{
  printf("File Begin Before Select Call\n");
  while (select_call(Server->sock, MAX_RECV_WAIT_TIME_S, MAX_RECV_WAIT_TIME_US))
    {
      if(recv_pkt(RecvPkt, Server->sock, &(Server->remote), Server->buffsize))
	{
	  if (RecvPkt->Hdr->seq == Server->seq && file_begin_pkt(RecvPkt))
	    {
	      create_pkt(SendPkt, FILE_BEGIN_ACK, Server->seq, NULL, 0);
	      send_pkt(SendPkt, Server->sock, Server->remote);
	      printf("S_DONE for file begin\n");
	      return S_DONE;
	    }
	  if (init_pkt(RecvPkt))
	    return S_OPEN_FILE;
	}
    }
  printf("Select Call Failed\n");
  return S_FINISH;
}

STATE wait_on_ack(sock *Server, pkt *RecvPkt)
{
  while (select_call(Server->sock, MAX_TRY_WAIT_TIME_S, MAX_TRY_WAIT_TIME_US))
    {
      if(recv_pkt(RecvPkt, Server->sock, &(Server->remote), Server->buffsize))
	if (RecvPkt->Hdr->seq == Server->seq)
	    return S_DONE;
      
    }
  return S_TIMEOUT_ON_ACK;
}

void transfer_setup(sock *Server, sock *Old_Server)
{
  uint32_t len = sizeof(Server->local);
  *Server = *Old_Server;
  Server->sock = s_socket(SOCK_DOMAIN, SOCK_TYPE, DEFAULT_PROTOCOL);

  Server->local.sin_family = SOCK_DOMAIN;
  Server->local.sin_addr.s_addr = htonl(INADDR_ANY); //???
  Server->local.sin_port = htons(DEFAULT_PORT);

  s_bind(Server->sock, (struct sockaddr *)&(Server->local), 
  	 sizeof(Server->local));

  s_getsockname(Server->sock, (struct sockaddr *)&(Server->local), 
		(socklen_t *)&len);
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

/*
 * CPE464 Library - bindMod
 *
 * Identical usage as bind, but allows the grader to check/change the
 * actions the bind call does.
 *
 * Simply use bindMod wherever bind would be used
 */

#ifndef _BINDMOD_H_
#define _BINDMOD_H_

#include <sys/types.h>
#include <sys/socket.h>

#ifdef __cplusplus
extern "C" {
#endif

int bindMod(int sockfd, const struct sockaddr *addr, socklen_t addrlen);

#ifdef __cplusplus
}
#endif

#endif


/* 
 * CPE464 Library - checksum
 *
 * Checksum declaration 
 * shadows@whitefang.com
 *
 * Simple call in_cksum with a memory location and it will calculate
 * the checksum over the requested length. The results are turned in 
 * a 16-bit, unsigned short
 */

#ifdef __cplusplus
extern "C" {
#endif

unsigned short in_cksum(unsigned short *addr, int len);

#ifdef __cplusplus
}
#endif

/*
 * CPE464 Library - selectMod
 *
 * Identical usage as select, but allows the grader to check/change the
 * actions the select call does.
 *
 * Simply use selectMod wherever select would be used
 */

#ifndef _SELECTMOD_H_
#define _SELECTMOD_H_

#include <sys/select.h>

#ifdef __cplusplus
extern "C" {
#endif

int selectMod(
        int nfds,
        fd_set *readfds,
        fd_set *writefds,
        fd_set *exceptfds,
        struct timeval *timeout);

#ifdef __cplusplus
}
#endif

#endif


/*
 * CPE464 Library - sendErr
 *
 * Provides a mechanism to create simulated errors for a TCP-like connection

  
In your main()

To use the sendErr(...) function you must call sendErr_init(...) first.
sendErr_init(...) initializes fields that are used by sendErr(...).  You
should only call sendErr_init(...) once in each program.  

sendErr_init(...)
      error_rate - between 0 and less than 1 (0 means no errors)
      drop_flag - should packets be dropped (DROP_ON or DROP_OFF)
      flip_flag - should bits be flipped (FLIP_ON or FLIP_OFF)
      debug_flag - print out debug info (DEBUG_ON or DEBUG_OFF)
      random_flag - if set to RSEED_ON, then seed is randon, if RSEED_OFF then seed is not random
      ex:

      sendErr_init(.1, DROP_ON, FLIP_OFF, DEBUG_ON, RSEED_ON);

*/

#ifndef SEND_ERR_ENUM_
#define SEND_ERR_ENUM

#define DEBUG_ON 1
#define DEBUG_OFF 0
#define FLIP_ON 1
#define FLIP_OFF 0
#define DROP_ON 1
#define DROP_OFF 0
#define RSEED_ON 1
#define RSEED_OFF 0

#endif

#ifndef SENDERR_H_
#define SENDERR_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/socket.h>

int sendErr_init(
        double error_rate,
        int drop_flag,
        int flip_flag,
        int debug_flag,
        int random_flag);

int sendErr(int s, void *msg, int len, unsigned int flags);
  
#ifdef __cplusplus
}
#endif

#endif


/*
 * CPE464 Library - sendtoErr
 *
 * Provides a mechanism to create simulated errors for a UDP-like connection
  
In your main()

To use the sendtoErr(...) function you must call sendtoErr_init(...) first.
sendtoErr_init(...) initializes fields that are used by sendtoErr(...).  You
should only call sendtoErr_init(...) once in each program.  

sendErr_init(error_rate, drop_flag, flip_flag, debug_flag, random_flag)
      error_rate - float between 0 and less than 1 (0 means no errors)
      drop_flag - should packets be dropped (DROP_ON or DROP_OFF)
      flip_flag - should bits be flipped (FLIP_ON or FLIP_OFF)
      debug_flag - pring out debug info (DEBUG_ON or DEBUG_OFF)
      random_flag - causes srand48 to use a time based seed (RSEED_ON or RSEED_OFF)
                    (see man srand48 - RSEED_OFF makes your program
		    runs more repeatable)

      If you don't know what to set random_flag to, use RSEED_OFF for initial
      debugging.  Once your program works, try RSEED_ON.  This will make the
      drop/flip pattern random between program runs.
		    
      ex:


      sendtoErr_init(.1, DROP_ON, FLIP_OFF, DEBUG_ON, RSEED_OFF);

*/

#ifndef SEND_ERR_ENUM_
#define SEND_ERR_ENUM

#define DROP_ON 1
#define DROP_OFF 0
#define FLIP_ON 1
#define FLIP_OFF 0
#define DEBUG_ON 1
#define DEBUG_OFF 0
#define RSEED_ON 1
#define RSEED_OFF 0 

#endif

#ifndef SENDTOERR_H_
#define SENDTOERR_H_

#include <sys/socket.h>

#ifdef __cplusplus
extern "C" {
#endif
    
  int sendtoErr(int s, void *msg, int len,  unsigned  int flags, const struct sockaddr *to, int tolen);
  int sendtoErr_init(double error_rate, int drop_flag, int flip_flag, int debug_flag, int random_flag);

#ifdef __cplusplus
}
#endif

#endif



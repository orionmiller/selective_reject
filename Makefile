# Author: Orion Miller (aomiller)

CC = gcc
CFLAGS = -g -Wall -Werror -Wpacked #-MD

#Dependencies
RCOPY_DEPEND = rcopy.c rcopy.h librcp.o
SERVER_DEPEND = server.c server.h librcp.o
LIB_RCP_DEPEND = librcp.c librcp.h util.o #checksum.c checksum.h # cpe464.h libcpe464.1.2.a
CPE_464_LIB_DEPEND = cpe464.h libcpe464.1.2.a
UTIL_DEPEND = util.c util.h

#Sources
RCOPY_SRCS = rcopy.c librcp.o
SERVER_SRCS = server.c librcp.o
LIB_RCP_SRCS = librcp.c util.o #checksum.c
UTIL_SRCS = util.c


all: rcopy server #server rcopy librcp.o util.o

rcopy: rcopy.* libsrj.* util.*
	$(CC) $(CFLAGS) rcopy.c libsrj.c util.c ./libcpe464.1.2.a -o $@

server: server.* libsrj.* util.*
	$(CC) $(CFLAGS) server.c libsrj.c util.c ./libcpe464.1.2.a -o $@

# rcopy: $(RCOPY_DEPEND)
# 	$(CC) $(CFLAGS) $(RCOPY_SRCS) -o $@

# server: $(SERVER_DEPEND)
# 	$(CC) $(CFLAGS) $(SERVER_SRCS) -o $@

# librcp.o: $(LIB_RCP_DEPEND)
# 	$(CC) $(CFLAGS) $(LIB_RCP_SRCS) -o $@

# util.o: $(UTIL_DEPEND)
# 	$(CC) $(CFLAGS) $(UTIL_SRCS) -o $@

# cpe464lib: 
# 	@make -f ./cpe464_lib/lib.mk
# 	@rm -f example.mk

clean:
	@rm -f *~ *.o *.d

allclean:
	@rm -f *~ rcopy server *~ *.o example.mk *.d

test:
	@echo "Needs a test functionality"


#-include *.d
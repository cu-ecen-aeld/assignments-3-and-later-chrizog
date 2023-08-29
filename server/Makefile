CROSS_COMPILE := 
CC := gcc
COMPILER := $(CROSS_COMPILE)$(CC)
CC_FLAGS = -g -Wall -Werr

aesdsocket:	aesdsocket.c
		$(COMPILER) -o aesdsocket aesdsocket.c

clean:	
		@if [ -e aesdsocket ]; then \
			rm aesdsocket; \
		fi
		
all:	clean aesdsocket

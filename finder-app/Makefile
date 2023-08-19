CROSS_COMPILE := x86_64-linux-gnu-
CC := gcc
COMPILER := $(CROSS_COMPILE)$(CC)

writer:	writer.c
		$(COMPILER) -o writer writer.c

clean:	
		@if [ -e writer ]; then \
			rm writer; \
		fi

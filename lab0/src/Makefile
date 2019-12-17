CC = gcc

all: lab0

lab0: lab0.c
	$(CC) -g -Wall -Wextra -o lab0 lab0.c

clean: 
	rm -f lab0-304965058.tar.gz lab0

dist:  
	tar -czvf lab0-304965058.tar.gz lab0.c Makefile README backtrace.png breakpoint.png test

check: 
	(./test && echo 0 > /dev/null) || (echo 1 > /dev/null)

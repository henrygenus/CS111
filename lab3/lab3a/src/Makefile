all: lab3

dist: lab3
	tar -czvf lab3a-304965058.tar.gz main.c lab3.h lab3.c ext2_fs.h README Makefile

clean: 
	rm -rf lab3a lab3a-304965058.tar.gz

lab3: 
	gcc -g -Wall -Wextra -o lab3a main.c lab3.c -lm

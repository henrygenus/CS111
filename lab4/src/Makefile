all: lab4b lab4c lab4c_tcp lab4c_tls

dist: 
	tar -czf lab4c-304965058.tar.gz constants.h lab4b_src lab4c_src README Makefile compile_b.sh 

clean: 
	rm -rf lab4c-304965058.tar.gz lab4b  lab4c lab4c_tcp lab4c_tls

lab4b:
	gcc -Wall -Wextra lab4b_src/lab4b.c lab4b_src/main_b.c -lm -o lab4b -lmraa

lab4c: lab4b
	gcc -Wall -Wextra lab4c_src/main_c.c lab4c_src/lab4c.c -o lab4c -lssl -lcrypto -lz 

lab4c_tcp: lab4c
	ln -s lab4c lab4c_tcp 

lab4c_tls: lab4c
	ln -s lab4c lab4c_tls

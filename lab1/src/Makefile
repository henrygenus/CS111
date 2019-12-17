all : simpsh

simpsh : simpsh.c
	gcc -g -Wall -Wextra -o simpsh simpsh.c

clean : 
	rm -f simpsh lab1-304965058.tar.gz

dist : 
	tar -czvf lab1-304965058.tar.gz simpsh.c test.sh Makefile README report.pdf

check : simpsh
	(./test.sh && echo 0 > /dev/null) || (echo 1 > /dev/null)

SD='304965058'
check_all : dist
	(./TAtest 304965058 && echo 0 > /dev/null) || (echo 1 > /dev/null)

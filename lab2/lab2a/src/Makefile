all: lab2_add lab2_list

build: lab2_add lab2_list

tests: lab2_add lab2_list
	(make addtest && make listtest && echo 0 > /dev/null) || (echo 1 > /dev/null)

addtest: lab2_add
	./add_script > lab2_add.csv

listtest: lab2_list
	./list_script > lab2_list.csv 

graphs: tests
	/usr/local/cs/bin/gnuplot ./lab2_add.gp
	/usr/local/cs/bin/gnuplot ./lab2_list.gp

dist: graphs
	tar -czf lab2a-304965058.tar.gz README Makefile lab2_add.c lab2_list.c\
	    lab2_add.gp lab2_list.gp lab2_add.csv lab2_list.csv \
	    lab2_add-1.png lab2_add-2.png lab2_add-3.png lab2_add-4.png lab2_add-5.png \
	    lab2_list-1.png lab2_list-2.png lab2_list-3.png lab2_list-4.png 
	    SortedList.c SortedList.h lab2.h add_script list_script		

clean: 
	rm -f lab2_add lab2_list lab2a-304965058.tar.gz \
#	    lab2_add-1.png lab2_add-2.png lab2_add-3.png lab2_add-4.png lab2_add-5.png \
#	    lab2_list-1.png lab2_list-2.png lab2_list-3.png lab2_list-4.png \
#	    lab2_add.csv lab2_list.csv

lab2_add: 
	gcc -g -Wall -Wextra -pthread -o lab2_add lab2_add.c 

lab2_list:
	gcc -g -Wall -Wextra -pthread -o lab2_list SortedList.c lab2_list.c 

clean_all: 
	rm -f lab2_add lab2_list lab2a-304965058.tar.gz \
	    lab2_add-1.png lab2_add-2.png lab2_add-3.png lab2_add-4.png lab2_add-5.png \
	    lab2_list-1.png lab2_list-2.png lab2_list-3.png lab2_list-4.png \
	    lab2_add.csv lab2_list.csv

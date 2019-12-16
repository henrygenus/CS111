all: lab2_list

build: lab2_list

tests: lab2_list
	(./run_tests > lab2b_list.csv && echo 0 > /dev/null) || (echo 1 > /dev/null)

profile: lab2_list
	LD_PRELOAD=/lib64/libprofiler.so CPUPROFILE=./raw.gpref \
		./lab2_list --threads=12 --iterations=1000 --sync=s  &> /dev/null
	/bin/pprof --text ./lab2_list ./raw.gpref > profile.out 2> /dev/null
	/bin/pprof --list=thread_action ./lab2_list ./raw.gpref >> profile.out 2>/dev/null

graphs: tests
	(/usr/local/cs/bin/gnuplot ./lab2b_list.gp && echo 0 > /dev/null) || (echo 1 > /dev/null)

dist: output
	tar -czf lab2b-304965058.tar.gz README Makefile lab2_list.c \
	    SortedList.c SortedList.h lab2.h run_tests	\
	    lab2b_1.png lab2b_2.png lab2b_3.png lab2b_4.png lab2b_5.png \
	    lab2b_list.gp lab2b_list.csv profile.out 

tarball:
	tar -czf lab2b-304965058.tar.gz README Makefile lab2_list.c \
            SortedList.c SortedList.h lab2.h run_tests  \
            lab2b_1.png lab2b_2.png lab2b_3.png lab2b_4.png lab2b_5.png \
            lab2b_list.gp lab2b_list.csv profile.out

clean: 
	rm -f lab2_list lab2b-304965058.tar.gz lab2_list.csv raw.gpref

lab2_list:
	gcc -g -Wall -Wextra -pthread -o lab2_list SortedList.c lab2_list.c 

clean_all:
	rm -f lab2_list lab2b-304965058.tar.gz lab2_list.csv raw.gpref profile.out \
	lab2b_1.png lab2b_2.png lab2b_3.png lab2b_4.png lab2b_5.png 

output: tests graphs profile 

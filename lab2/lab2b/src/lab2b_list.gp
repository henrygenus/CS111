#! /usr/bin/gnuplot
#
# purpose:
#	 generate data reduction graphs for the multi-threaded list project
#
# input: lab2_list.csv
#	1. test name
#	2. # threads
#	3. # iterations per thread
#	4. # lists
#	5. # operations performed (threads x iterations x (ins + lookup + delete))
#	6. run time (ns)
#	7. run time per operation (ns)
#	8. wait time per operation (ns)
#
# output:
#	lab2b_1.png ... Throughput of Mutex and Spin Locks
#	lab2b_2.png ... Efficiency of Mutex Locks
#	lab2b_3.png ... Successful Iterations with Sublists
#	lab2b_4.png ... Throughput of Mutex Lock with Sublists
#	lab2b_5.png ... Throughput of Spin Lock with Sublists
#
# Note:
#	Managing data is simplified by keeping all of the results in a single
#	file.  But this means that the individual graphing commands have to
#	grep to select only the data they want.
#
#	Early in your implementation, you will not have data for all of the
#	tests, and the later sections may generate errors for missing data.
#

# general plot parameters
set terminal png
set datafile separator ","

# operations per second w/ mutex vs spin-lock
set title "List-1: Throughput of Mutex and Spin Locks\n{/*0.5 1000 Iterations, 1 Sublist, No Yielding}"
set xlabel "Threads"
set xrange [1:24]
set xtics (1,2,4,8,12,16,24)
set ylabel "Operations per Second"
set logscale y 10
set output 'lab2b_1.png'

# grep out only mutex and spin locks with a thousand iterations one list and no yield
plot \
     "< grep 'list-none-m,[0-9]*,1000,1,' lab2b_list.csv" using ($2):(1000000000/($7)) \
	title 'mutex' with linespoints lc rgb 'red', \
     "< grep 'list-none-s,[0-9]*,1000,1,' lab2b_list.csv" using ($2):(1000000000/($7)) \
	title 'spin-lock' with linespoints lc rgb 'green'

# wait-for-lock time and time per operation vs threads
set title "List 2: Efficiency of Mutex Locks\n{/*0.5 1000 Iterations, 1 Sublist, No Yielding}"
set xlabel "Threads"
set ylabel "Time per Operation (ns)"
unset logscale y
set output 'lab2b_2.png'

# grep out only mutex locks, once for wait time and once for operation time
plot \
     "< grep 'list-none-m,[0-9]*,1000,1,' lab2b_list.csv" using ($2):($7) \
        title 'average time per operation' with linespoints lc rgb 'green', \
     "< grep 'list-none-m,[0-9]*,1000,1,' lab2b_list.csv" using ($2):($8) \
        title 'average wait time per operation' with linespoints lc rgb 'red'


set title "List 3: Successful Iterations with Sublists\n{/*0.5 4 Sublists, Insert and Delete Yielding}"
set xlabel "Threads"
set xrange [1:16]
set xtics (1,4,8,12,16)
set ylabel "Successful Iterations"
unset logscale y
set yrange [0:80]
set ytics (10,20,30,40,50,60,70,80)
set output 'lab2b_3.png'

# grep out only yield=id & list=4 & no, mutex, and spin locking
plot \
     "< grep 'list-id-m,[0-9]*,[0-9]*,4,' lab2b_list.csv" using ($2):($3) \
     	title 'mutex-lock, yield-id' with points lc rgb "green", \
     "< grep 'list-id-s,[0-9]*,[0-9]*,4,' lab2b_list.csv" using ($2):($3) \
     	title "spin-lock, yield-id" with points lc rgb "red", \
     "< grep 'list-id-none,[0-9]*,[0-9]*,4,' lab2b_list.csv" using ($2):($3) \
     	title 'unprotected, yield-id' with points lc rgb "blue"

set title "List 4: Throughput of Mutex Lock with Sublists\n{/*0.5 No Yielding, 1000 Iterations}"
set xlabel "Threads"
set xrange [1:16]
set xtics (1,2,4,8,12,16)
set ylabel "Operations per Second"
unset yrange
unset ytics
set ytics auto
set output "lab2b_4.png"

# grep out mutex locks w/ no yield for each list count, 1000 iterations
plot \
     "< grep 'list-none-m,[0-9]*,1000,1,' lab2b_list.csv" using ($2):(1000000000/($7)) \
     	title '1 List' with linespoints lc rgb 'green', \
     "< grep 'list-none-m,[0-9]*,1000,4,' lab2b_list.csv" using ($2):(1000000000/($7)) \
        title '4 Lists' with linespoints lc rgb 'blue', \
     "< grep 'list-none-m,[0-9]*,1000,8,' lab2b_list.csv" using ($2):(1000000000/($7)) \
        title '8 Lists' with linespoints lc rgb 'red', \
     "< grep 'list-none-m,[0-9]*,1000,16,' lab2b_list.csv" using ($2):(1000000000/($7)) \
        title '16 Lists' with linespoints lc rgb 'violet'

set title "List 4: Throughput of Spin Lock with Sublists\n{/*0.5 No Yielding, 1000 Iterations}"
set output "lab2b_5.png"
# grep out spin locks w/ no yield for each list count, 1000 iterations
plot \
     "< grep 'list-none-s,[0-9]*,1000,1,' lab2b_list.csv" using ($2):(1000000000/($7)) \
        title '1 List' with linespoints lc rgb 'green', \
     "< grep 'list-none-s,[0-9]*,1000,4,' lab2b_list.csv" using ($2):(1000000000/($7)) \
        title '4 Lists' with linespoints lc rgb 'blue', \
     "< grep 'list-none-s,[0-9]*,1000,8,' lab2b_list.csv" using ($2):(1000000000/($7)) \
        title '8 Lists' with linespoints lc rgb 'red', \
     "< grep 'list-none-s,[0-9]*,1000,16,' lab2b_list.csv" using ($2):(1000000000/($7)) \
        title '16 Lists' with linespoints lc rgb 'violet'

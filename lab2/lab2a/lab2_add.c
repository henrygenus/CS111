#include "lab2.h" //constants, helper functions
#include <stdio.h> //fprintf
#include <stdlib.h> //exit
#include <getopt.h> //getopt
#include <string.h> // strerror
#include <errno.h> //errno
#include <time.h> //gettime, clock constant
#include <pthread.h> //threads

#define MAX_TEST_LENGTH 15

void add(long long *pointer, long long value);
void atomic_add(long long* pointer, long long value);
void* thread_action(void *pointer);
inline void lock(lock_info *lock, char lock_type);
inline void unlock(lock_info *lock, char lock_type);


int main(int argc, char** argv) {
  int opt, longindex, ctr, noperations, ret = 0, nthreads = 1, niterations = -1, spin_lock = 0;
  char test[MAX_TEST_LENGTH] = "add", lock_type[6] = "-none";
  struct timespec* start_time = &(struct timespec){0,0};
  struct timespec* end_time = &(struct timespec){0,0};
  long long counter = 0; long time;
  pthread_t* threads; 
  pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER; 
  lock_info lock_union; 

  // get thread and iteration number
  while(1) {
    if ((opt = getopt_long(argc, argv, "", longopts, &longindex)) == -1)
      break;
    switch(opt) {
    case THREADS:
      nthreads = (optarg == NULL ? 1 : string_to_int(optarg));
      break;
    case ITERATIONS:
      niterations = (string_to_int(optarg));
      break;
    case YIELD:
      opt_yield = 1;
      break;
    case SYNC:
      if (strcmp(optarg, "m") == 0) lock_union.mlock = &lock;
      else if (strcmp(optarg, "s") == 0) lock_union.slock = &spin_lock;
      else if (strcmp(optarg, "c") == 0) {;}
      else {
	error_string("Bad sync parameter", optarg, 1);
      }
      strcpy(lock_type, "-"); strcat(lock_type, optarg);        
      break;
    default:
      exit(1);
      break;
    }
  }
  // check thread and iteration numbers
  if (nthreads <= 0)
    error_int("Invalid Thread Number", nthreads, 1);
  else if (niterations < 0)
    error_int("Invalid Iteration Number", niterations, 1);

  // get test string and lock ready
  if (opt_yield) strcat(test, "-yield");
  strcat(test, lock_type);

  //get pre-action time
  if(clock_gettime(CLOCK_REALTIME, start_time) == -1)
    error_string(strerror(errno), "pre-operation time", 1);

  //create threads
  threads = (pthread_t*)malloc(nthreads*sizeof(pthread_t));
  for(ctr=0; ctr < nthreads; ctr++) 
    pthread_create(&threads[ctr], NULL, thread_action, 
		   &(struct add_info){&counter, niterations, lock_type[1], lock_union});

  // collect threads
  for(ctr=0; ctr < nthreads; ctr++) 
    if(pthread_join(threads[ctr], NULL))
      error_int(strerror(errno), threads[ctr], 2);

  // get accumulated time
  if(clock_gettime(CLOCK_REALTIME, end_time) == -1)
    error_string(strerror(errno), "post-operation time", 2);
  time = (end_time->tv_sec - start_time->tv_sec)*1000000000
    + (end_time->tv_nsec - start_time->tv_nsec);
  noperations = nthreads*niterations*2;

  // output and exit
  fprintf(stdout, "%s,%i,%i,%i,%li,%li,%lli\n", test,
	  nthreads, niterations, noperations, time, time/(long)noperations, counter);
  if(threads != NULL)
    free(threads);
  exit(ret);
}


///////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////// FUNCTION IMPLEMENTATION ///////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////


void add(long long *pointer, long long value) {
  long long sum = *pointer + value;
  if (opt_yield)
    sched_yield();
  *pointer = sum;
}

void atomic_add(long long* pointer, long long value) {
  long long prev, sum;
  do {
    prev = *pointer;
    sum = prev + value;
    if (opt_yield)
      sched_yield();
  } while(__sync_val_compare_and_swap(pointer, prev, sum) != prev);
}

void* thread_action(void *pointer) {
  int ctr;
  struct add_info* p = (struct add_info*)pointer;
  void (*fcn)(long long *pointer, long long value) = (p->lock_type == 'c') ? &atomic_add : &add;
  for(ctr = 0; ctr < p->iterations; ctr++) {
    lock(&p->lock, p->lock_type);
    (*fcn)(p->pointer, 1);
    (*fcn)(p->pointer, -1);
    unlock(&p->lock, p->lock_type);
  }
  return NULL;
}

void lock(lock_info *lock, char lock_type) {
  if (lock_type == 'm') pthread_mutex_lock(lock->mlock);
  else if (lock_type == 's') while (__sync_lock_test_and_set(lock->slock, 1)) ;
}

void unlock(lock_info *lock, char lock_type) {
  if (lock_type == 'm') pthread_mutex_unlock(lock->mlock);
  else if (lock_type == 's') __sync_lock_release(lock->slock);
}



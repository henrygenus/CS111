#include "lab2.h"
#include "SortedList.h"

#include <stdio.h> //fprintf                                                                       
#include <stdlib.h> //exit                                                                         
#include <getopt.h> //getopt                                                                       
#include <string.h> // strerror                                                                    
#include <errno.h> //errno                                                                         
#include <time.h> //gettime, clock constant                                                        
#include <pthread.h> //threads                
#include <stdlib.h> //random
#include <signal.h> // signal(2)

#define OPS_PER_ITERATION 3

struct list_info 
{ 
  SortedList_t *list; 
  SortedListElement_t **elements; 
  int nelements;
  lock_info lock;
  char lock_type;
  int *ret;
};

void handler(int sig) { 
  error_string("ERROR", "Segmentation Fault.", sig == 2 ? sig : 2); }

void* thread_action(void *pointer);
inline void set_string(char* string, int length);
inline void lock(lock_info* lock, char lock_type);
inline void unlock(lock_info* lock, char lock_type);


int main(int argc, char** argv) {
  //utility variables
  int ctr, opt, len, longindex, ret=0, nthreads = 1, niterations = -1, noperations; 
  char name[16] = "list"; char flags[6] = "-none"; char lock_type[6] = "-none";
  // time related variables
  struct timespec* start_time = &(struct timespec){0,0};
  struct timespec* end_time = &(struct timespec){0,0};
  long long time; 
  // lock variables
  int spin_lock = 0; pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER; lock_info lock_union;
  //list variables
  SortedList_t *list = (SortedList_t*)malloc(sizeof(SortedList_t)); 
  list->key = NULL; list->next = list; list->prev = list;
  char **keys; int *indeces; pthread_t *threads;
  SortedListElement_t **elements; struct list_info *info; 
  // optional: print commands while executing
  /*
  for (ctr = 0; ctr < argc; ctr++)
    fprintf(stderr, "%s ", argv[ctr]);
  fprintf(stderr, "\n");
  */
  opt_yield = 0;
  // get thread and iteration number                                                               
  while(1) {
    if ((opt = getopt_long(argc, argv, "", longopts, &longindex)) == -1)
      break;
    switch(opt) {
    case THREADS:
      nthreads = (optarg == NULL ? 1 : string_to_int(optarg));
      break;
    case ITERATIONS:
      niterations = (optarg == NULL ? 1 : string_to_int(optarg));
      break;
    case YIELD:
      ctr = 0;
      do {
	if (optarg[ctr] == 'i') opt_yield |= INSERT_YIELD;
	else if (optarg[ctr] == 'd') opt_yield |= DELETE_YIELD;
	else if (optarg[ctr] == 'l') opt_yield |= LOOKUP_YIELD;
	else if (optarg[ctr] == '\0') break;
	else error_string("Bad parameter", optarg, 1); 
      } while (optarg[++ctr]);
      strcpy(flags, "-");
      if (! opt_yield) error_string("Empty options for", longopts[longindex].name, 1);
      if (opt_yield & INSERT_YIELD) strcat(flags, "i");
      if (opt_yield & DELETE_YIELD) strcat(flags,"d");
      if (opt_yield & LOOKUP_YIELD) strcat(flags, "l");
      break;
    case SYNC:
      if (strcmp(optarg, "m") == 0) lock_union.mlock = &lock;
      else if (strcmp(optarg, "s") == 0) lock_union.slock = &spin_lock;
      else error_string("Bad parameter", optarg, 1);
      strcpy(lock_type, "-"); strcat(lock_type, optarg);        
      break;      
    default:
      exit(1);
      break;
    }
  }  

  // check thread and iteration numbers                                                            
  if (nthreads <= 0) error_int("Invalid Thread Number", nthreads, 1); 
  else if (niterations < 0) error_int("Invalid iteration number", nthreads, 1); 

  // set info
  strcat(name, flags);
  strcat(name, lock_type);
  noperations = nthreads*niterations*OPS_PER_ITERATION;

  // create list elements
  elements = (SortedListElement_t**)malloc(nthreads*niterations*sizeof(SortedListElement_t*));
  keys = (char**)malloc(nthreads*niterations*sizeof(char*));
  for (ctr = 0; ctr < nthreads*niterations; ctr++) {
    len = (5);
    keys[ctr] = (char*)malloc(len+1 * sizeof(char));
    set_string(keys[ctr], len);
    elements[ctr] = malloc(sizeof(SortedListElement_t));
    *elements[ctr] = (SortedListElement_t){NULL, NULL, keys[ctr]};
  }

  // allocate space for thread info
  indeces = (int*)malloc(nthreads*sizeof(int));
  threads = (pthread_t*)malloc(nthreads*sizeof(pthread_t));
  info = (struct list_info*)malloc(nthreads*sizeof(struct list_info));


  // ignore  seg faults for threads
  signal(SIGSEGV, handler);

  //get pre-action time                                                                           
  if(clock_gettime(CLOCK_REALTIME, start_time) == -1) 
    error_string(strerror(errno), "pre-operation time", 1);
  
  // create threads
  for(ctr=0; ctr < nthreads; ctr++) {
    indeces[ctr] = ctr;
    info[ctr] = (struct list_info){list, &elements[indeces[ctr]*niterations], niterations, 
				   lock_union, lock_type[1], &ret};
    pthread_create(&threads[ctr], NULL, thread_action, &info[ctr]);
  }

  // collect threads                                                                              
  for(ctr=0; ctr < nthreads; ctr++) 
    if (pthread_join(threads[ctr], NULL)) 
      error_int(strerror(errno), threads[ctr], 0);
      

  // get accumulated time                                                                         
  if(clock_gettime(CLOCK_REALTIME, end_time) == -1)
    error_string(strerror(errno), "post-operation time", 1);
  else time = (end_time->tv_sec - start_time->tv_sec)*1000000000
	 + (end_time->tv_nsec - start_time->tv_nsec);

  // reset seg fault response
  signal(SIGSEGV, SIG_DFL);

  // sanity check
  if ((len = SortedList_length(list)) != 0)  
    error_int("List length is not 0\n", len, 2);

  // output and exit                                                                              
  fprintf(stdout, "%s,%i,%i,%i,%i,%lli,%lli\n", name,
          nthreads, niterations, 1, noperations, time, time/(long)noperations);


  // guarentee output and exit
  fflush(NULL);
  exit(ret);
}

void *thread_action(void *pointer) {
  int ctr; struct list_info *p = pointer; SortedListElement_t *ptr;
  lock(&p->lock, p->lock_type);
  for(ctr = 0; ctr < p->nelements; ctr++) 
    SortedList_insert(p->list, p->elements[ctr]); 
  unlock(&p->lock, p->lock_type);

  lock(&p->lock, p->lock_type);
  if ((ctr = SortedList_length(p->list)) < 0) 
    error_int("Corrupted List", ctr, 2);
  unlock(&p->lock, p->lock_type);

  lock(&p->lock, p->lock_type);
  for(ctr = 0; ctr < p->nelements; ctr++)  {
    ptr = SortedList_lookup(p->list, p->elements[ctr]->key);
    if (ptr == NULL || SortedList_delete(ptr) == 1) 
      error_string("Key Not Found", p->elements[ctr]->key, 2);
  }
  unlock(&p->lock, p->lock_type);

  return NULL;
}

void set_string(char* string, int length) { 
  int ctr;
  for(ctr = 0; ctr < length; ctr++)
    string[ctr] = (rand() % 94 + 32);
  string[length] = '\0';
}

void lock(lock_info *lock, char lock_type) {
  if (lock_type == 'm') pthread_mutex_lock(lock->mlock);
  else if (lock_type == 's') while (__sync_lock_test_and_set(lock->slock, 1)) ;
}

void unlock(lock_info *lock, char lock_type) {
  if (lock_type == 'm') pthread_mutex_unlock(lock->mlock);
  else if (lock_type == 's') __sync_lock_release(lock->slock);
}

#include "lab2.h"
#include <stdio.h> //fprintf
#include <stdlib.h> //exit
#include <string.h> // strerror
#include <errno.h> //errno     
#include <time.h> //gettime, clock constant
#include <pthread.h> //threads
#include <signal.h> // signal(2)

void* thread_action(void *pointer);
inline void lock(node_lock *lock);
inline void unlock(node_lock *lock);
inline unsigned int hash(const char *str);

int opt_yield = 0;

int main(int argc, char** argv) {

  //utility variables
  int ctr, opt, len, longindex, noperations, nthreads, niterations, nlists, nelements; 
  char name[16] = "list"; char flags[6] = "-none"; char lock_type[6] = "-none";

  // time related variables
  long long time = 0; long long wait_time = 0; long long *times;  
  struct timespec* start_time = &(struct timespec){0,0};
  struct timespec* end_time = &(struct timespec){0,0};

  // lock variables
  status lock_union; int* spinptr; pthread_mutex_t *mutexptr;
  
  //list variables
  lock_and_node **locked_lists; SortedList_t* list; node_lock **locks;
  char **keys; pthread_t *threads; SortedListElement_t **elements; struct list_info *info;

  // optional: print commands while executing
  /*  
  for (ctr = 0; ctr < argc; ctr++)
    fprintf(stderr, "%s ", argv[ctr]);
  fprintf(stderr, "\n");
  */

  // get thread and iteration number
  while(1) {
    if ((opt = getopt_long(argc, argv, "", longopts, &longindex)) == -1) break;
    switch(opt) {
    case THREADS:
      nthreads = (optarg == NULL ? 1 : string_to_int(optarg));
      break;
    case ITERATIONS:
      niterations = (optarg == NULL ? 1 : string_to_int(optarg));
      break;
    case LISTS:
      nlists = (optarg == NULL ? 1 : string_to_int(optarg));
      break;
    case YIELD:
      ctr = 0;
      do {
	if (optarg[ctr] == I_YIELD) opt_yield |= INSERT_YIELD;
	else if (optarg[ctr] == D_YIELD) opt_yield |= DELETE_YIELD;
	else if (optarg[ctr] == L_YIELD) opt_yield |= LOOKUP_YIELD;
	else if (optarg[ctr] == '\0') break;
	else error_string("Bad parameter", optarg, 1); 
      } while (optarg[++ctr]);
      strcpy(flags, "-");
      if (! opt_yield) error_string("Empty options for", longopts[longindex].name, 1);
      if (opt_yield & INSERT_YIELD) strcat(flags, INSERT);
      if (opt_yield & DELETE_YIELD) strcat(flags, DELETE);
      if (opt_yield & LOOKUP_YIELD) strcat(flags, LOOKUP);
      break;
    case SYNC:
      if (*optarg != MUTEX_LOCK && *optarg != SPIN_LOCK)
	error_string("Bad parameter", optarg, 1);
      strcpy(lock_type, "-");
      strcat(lock_type, optarg); 
      break;      
    default:
      exit(1);
      break;
    }
  }  

  // check thread and iteration numbers
  if (nthreads <= 0) error_int("Invalid thread number", nthreads, 1); 
  else if (niterations < 0) error_int("Invalid iteration number", nthreads, 1); 
  else if (nlists < 0) error_int("invalid list number", nlists, 1);

  // initialize data
  strcat(name, flags);
  strcat(name, lock_type);
  nelements = nthreads*niterations;
  noperations = nelements*OPS_PER_ITERATION;
  
  // allocate space 
  if (!(threads = (pthread_t*)malloc(nthreads*sizeof(pthread_t)))) exit(SYS_ERROR);
  if (!(info = (struct list_info*)malloc(nthreads*sizeof(struct list_info)))) EXIT(SYS_ERROR);

  if (!(keys = (char**)malloc(nelements*sizeof(char*)))) exit(SYS_ERROR);
  if (!(times = (long long*)malloc(nelements*sizeof(long long)))) exit(SYS_ERROR);
  if (!(elements = (SortedListElement_t**)malloc(nelements*sizeof(SortedListElement_t*))))
    exit(SYS_ERROR);

  if (!(locked_lists = (lock_and_node**)malloc(nlists*sizeof(lock_and_node*)))) exit(SYS_ERROR); 
  if (!(locks = (node_lock**)malloc(nlists*sizeof(node_lock*)))) exit(SYS_ERROR);

  // need to give memory & assign locks still
  for (ctr = 0; ctr < nlists; ctr++) {
    //make lock
    if (!(locks[ctr] = (node_lock*)malloc(sizeof(node_lock)))) exit(SYS_ERROR);
    // if there is one thread, a lock is pointless
    if (nthreads != 1) {      
      if (lock_type[1] == MUTEX_LOCK) {
	if (!(mutexptr = (pthread_mutex_t*)malloc(sizeof(pthread_mutex_t)))) exit(SYS_ERROR);
	lock_union.mlock = mutexptr;
	if (pthread_mutex_init(lock_union.mlock, NULL) == -1) exit(SYS_ERROR);
      }
      if (lock_type[1] ==  SPIN_LOCK) {
	if (!(spinptr = (int*)malloc(sizeof(int)))) exit(SYS_ERROR);
	lock_union.slock = spinptr;
	*lock_union.slock = 0;
      }
    }
    *locks[ctr] = (node_lock) {lock_union, nthreads == 1 ? NO_LOCK : lock_type[1]};
    
    // make list node
    if (!(list = (SortedList_t*)malloc(sizeof(SortedList_t)))) exit(SYS_ERROR);
    *list = (SortedList_t) {list, list, NULL};
    if (!(locked_lists[ctr] = (lock_and_node*)malloc(sizeof(lock_and_node)))) exit(SYS_ERROR);
    *locked_lists[ctr] = (lock_and_node) {*locks[ctr], list};
  }
  // make list elements
  for (ctr = 0; ctr < nelements; ctr++) {
    times[ctr] = 0;
    if (!(keys[ctr] = (char*)malloc(((len = get_length()) + 1) * sizeof(char)))) exit(SYS_ERROR);
    set_string(keys[ctr], len);
    if (!(elements[ctr] = (SortedListElement_t*)malloc(sizeof(SortedList_t)))) exit(SYS_ERROR);
    *elements[ctr] = (SortedListElement_t){NULL, NULL, keys[ctr]};
  }

  // ignore  seg faults for threads
  if (signal(SIGSEGV, handler) == SIG_ERR) exit(SYS_ERROR);

  //get pre-action time
  if (clock_gettime(CLOCK_REALTIME, start_time) == -1) exit(SYS_ERROR);

  // create threads
  for(ctr=0; ctr < nthreads; ctr++) {
    info[ctr] = 
      (struct list_info) {
      locked_lists,
      nlists,
      &elements[ctr*niterations], 
      niterations, 
      &times[ctr]
    };
    if (pthread_create(&threads[ctr], NULL, thread_action, &info[ctr])) exit(SYS_ERROR); 
  }

  // collect threads                                                                              
  for(ctr=0; ctr < nthreads; ctr++) 
    if (pthread_join(threads[ctr], NULL)) exit(SYS_ERROR);

  // get accumulated time                                                                        
  if(clock_gettime(CLOCK_REALTIME, end_time) == -1) exit(SYS_ERROR);
  else time = subtract_times(end_time, start_time);

  // reset seg fault response
  if (signal(SIGSEGV, SIG_DFL) == SIG_ERR) exit(SYS_ERROR);

  // post-action housekeeping & sanity check
  for (ctr = 0; ctr < nlists; ctr++) {
    // sanity check -- all lists have been deleted
    if ((len = SortedList_length(locked_lists[ctr]->list)) != 0) 
      error_int("List length is not 0", len, 2);

    free(locked_lists[ctr]->list);
    if (nthreads != 1) {
      if (lock_type[1] == MUTEX_LOCK) {
	if (pthread_mutex_destroy(locks[ctr]->state.mlock)) exit(SYS_ERROR);
	free(locks[ctr]->state.mlock);
      }
      else if (lock_type[1] == SPIN_LOCK) free(locks[ctr]->state.slock);
    }
    free(locks[ctr]);
    free(locked_lists[ctr]);
  }
  
  for (ctr = 0; ctr < nthreads*niterations; ctr++) {
    wait_time += times[ctr];
    free(keys[ctr]);
    free(elements[ctr]);
  }
  free(threads);
  free(info);
  free(elements);
  free(keys);
  free(times);

  // output
  fprintf(stdout, "%s,%i,%i,%i,%i,%lli,%lli,%lli\n", name,
	  nthreads, niterations, nlists, noperations,
	  time, time/(long)noperations, wait_time/((long)noperations));

  // guarentee output and exit
  fflush(NULL);
  exit(0);
}

void *thread_action(void *pointer) {
  int ctr, bucket; struct list_info *p = pointer; SortedListElement_t *ptr;
  struct timespec *pre_time = &(struct timespec) {0,0};
  struct timespec *post_time = &(struct timespec) {0,0};
  
  // insert items
  for(ctr = 0; ctr < p->nelements; ctr++) {
    bucket = hash(p->elements[ctr]->key) % p->nlists;
    if(p->lists[bucket]->locks.type != NO_LOCK) {
      clock_gettime(CLOCK_MONOTONIC, pre_time);
      lock(&p->lists[bucket]->locks);
      clock_gettime(CLOCK_MONOTONIC, post_time);
      *p->time += subtract_times(post_time, pre_time);
    }
    SortedList_insert(p->lists[bucket]->list, p->elements[ctr]); 
    if(p->lists[bucket]->locks.type != NO_LOCK) unlock(&p->lists[bucket]->locks);  
  }

  // get length
  for(ctr = 0; ctr < p->nelements; ctr++) {
    bucket = hash(p->elements[ctr]->key) % p->nlists;
    if(p->lists[bucket]->locks.type != NO_LOCK) {
      clock_gettime(CLOCK_MONOTONIC, pre_time);
      lock(&p->lists[bucket]->locks);
      clock_gettime(CLOCK_MONOTONIC, post_time);
      *p->time += subtract_times(post_time, pre_time);
    }
    if (SortedList_length(p->lists[bucket]->list) < 0)
      error_int("Corrupted List", bucket, 2);
    if(p->lists[bucket]->locks.type != NO_LOCK) unlock(&p->lists[bucket]->locks);
  }

  // search and delete
  for(ctr = 0; ctr < p->nelements; ctr++)  {
    bucket = hash(p->elements[ctr]->key) % p->nlists;
    if(p->lists[bucket]->locks.type != NO_LOCK) {
      clock_gettime(CLOCK_MONOTONIC, pre_time);
      lock(&p->lists[bucket]->locks);
      clock_gettime(CLOCK_MONOTONIC, post_time);
      *p->time += subtract_times(post_time, pre_time);
    }
    ptr = SortedList_lookup(p->lists[bucket]->list, p->elements[ctr]->key);
    if (ptr == NULL || SortedList_delete(ptr) == 1)
      error_string("Key Not Found", p->elements[ctr]->key, 2);
    if(p->lists[bucket]->locks.type != NO_LOCK) unlock(&p->lists[bucket]->locks);
 }
  return NULL;
}

void lock(node_lock *lock) {
  if (lock->type == MUTEX_LOCK) pthread_mutex_lock(lock->state.mlock);
  else if (lock->type == SPIN_LOCK) while (__sync_lock_test_and_set(lock->state.slock, 1)) ;
  else error_int("Could not acquire lock of type", lock->type, 1);
}

void unlock(node_lock *lock) {
  if (lock->type == MUTEX_LOCK) pthread_mutex_unlock(lock->state.mlock);
  else if (lock->type == SPIN_LOCK) __sync_lock_release(lock->state.slock);
  else error_int("Could not acquire lock of type", lock->type, 1);
}

unsigned int hash(const char *str)
{
  unsigned long hash = 5381;
  int c;
  while ((c = *str++))
    hash = (((hash << 5) + hash) + c); /* hash * 33 + c */
  return hash;
}

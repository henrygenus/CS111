#ifndef LAB2_H
#define LAB2_H

#include "SortedList.h"
#include <stdio.h> //fprintf
#include <ctype.h> //isdigit
#include <errno.h>
#include <string.h>
#include <getopt.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdlib.h> //random                                                    

#define MUTEX "m"
#define SPIN "s"
#define INSERT "i"
#define LOOKUP "l"
#define DELETE "d"
#define I_YIELD 'i'
#define L_YIELD 'l'
#define D_YIELD 'd'
#define THREADS 'a'
#define ITERATIONS 'b'
#define LISTS 'c'
#define YIELD 'd'
#define SYNC 'e'
#define NO_LOCK 'n'
#define SPIN_LOCK 's'
#define MUTEX_LOCK 'm'
#define OPS_PER_ITERATION 3
#define SYS_ERROR system_call_error()

static struct option longopts[] = {
  { .name = "threads",     .has_arg = optional_argument,  .flag = NULL,  .val = THREADS    },
  { .name = "yield",       .has_arg = optional_argument,  .flag = NULL,  .val = YIELD      },
  { .name = "iterations",  .has_arg = required_argument,  .flag = NULL,  .val = ITERATIONS },
  { .name = "sync",        .has_arg = required_argument,  .flag = NULL,  .val = SYNC       },
  { .name = "lists",        .has_arg = required_argument,  .flag = NULL,  .val = LISTS     },
  { .name = 0,             .has_arg = 0,                  .flag = 0,     .val = 0          }
};

// structs to allow passing of iterations and pointer

// handler for child thread seg faults
void handler(int sig) {
  write(STDERR_FILENO, "ERROR: Segmentation Fault.", 28);
  _exit(sig == 2 ? sig : 2);
}

// union that allows for spin and mutex locks
typedef union multiple_locks
{
  int *slock;
  pthread_mutex_t *mlock;
} status;                                                                                              

// struct that holds union and indicator of union type
typedef struct lock_and_status
{
  status state;
  char type;
} node_lock;

typedef struct node_and_lock
{
  node_lock locks;
  SortedList_t* list;
} lock_and_node;

// struct that stores all the info for list thread function
struct list_info
{
  lock_and_node **lists;
  int nlists;
  SortedList_t **elements;
  int nelements;
  long long *time;
};

// error message for system call failure
static inline int system_call_error() {
  perror(NULL);
  return 1;
}

// error message for when parameter is integer      
static inline void error_int(const char* message, int argument, int code) {
  fprintf(stderr, "%s: %d\n", message, argument);
  exit(code);
}

// error message for when parameter is string       
static inline void error_string(const char* message, const char* argument, int code) {
  fprintf(stderr, "%s: %s\n", message, argument);
  exit(code);
}

// subtracts two time structures
static inline long long subtract_times(struct timespec *end_time, struct timespec *start_time) {
  return (end_time->tv_sec - start_time->tv_sec)*1000000000
    + (end_time->tv_nsec - start_time->tv_nsec);
}

// convert string to integer
static inline int string_to_int(const char* string) {
  int ctr=0, sig=0, mult=1; char num;
  while (string[ctr] != 0)
    ctr++;
  while (--ctr >= 0) {
    if (!isdigit(string[ctr]))
      return -1;
    num = (string[ctr]-'0');
    sig += mult * (num);
    mult *= 10;
  }
  return sig;
}

static inline int get_length() { return (rand() % (MAX_LENGTH - MIN_LENGTH) + MIN_LENGTH); }

static inline void set_string(char* string, int length) {
  int ctr;
  for(ctr = 0; ctr < length; ctr++)
    string[ctr] = (rand() % 94 + 32);
  string[length] = '\0';
}                                                                                                      
  	     
#endif

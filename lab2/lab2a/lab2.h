#ifndef LAB2_H
#define LAB2_H

#include <stdio.h> //fprintf
#include <ctype.h> //isdigit
#include <errno.h>
#include <string.h>
#include <getopt.h>
#include <pthread.h>
#include <stdlib.h>

#define THREADS 'a'
#define ITERATIONS 'b'
#define YIELD 'c'
#define SYNC 'd'

#define MAX_LENGTH 10
#define MIN_LENGTH 3
int opt_yield;

static struct option longopts[] = {
  { .name = "threads",     .has_arg = optional_argument,  .flag = NULL,  .val = THREADS    },
  { .name = "yield",       .has_arg = optional_argument,  .flag = NULL,  .val = YIELD      },
  { .name = "iterations",  .has_arg = required_argument,  .flag = NULL,  .val = ITERATIONS },
  { .name = "sync",        .has_arg = required_argument,  .flag = NULL,  .val = SYNC       },
  { .name = 0,             .has_arg = 0,                  .flag = 0,     .val = 0          }
};

// structs to allow passing of iterations and pointer

typedef union tests
{
  int *slock;
  pthread_mutex_t *mlock;
} lock_info;

struct add_info
{ 
  long long* pointer; 
  int iterations; 
  char lock_type; 
  lock_info lock; 
};

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
	     
#endif

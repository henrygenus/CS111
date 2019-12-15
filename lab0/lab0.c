#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <getopt.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <string.h>

#define INPUT 'i'
#define OUTPUT 'o'
#define SEGFAULT 's'
#define CATCH 'c'
#define DUMP_CORE 'd'

void sigsegv_handler(int sig) {
  fprintf(stderr, "%s -- SIGSEGV Caught\n", strerror(sig));
  _exit(4);
}

void doFault() {
  char* ptr = NULL;
  *ptr = 0;
}

int main(int argc, char** argv) {
  // utility variables
  int ifd = 0, ofd = 1, longindex = 1, size, opt;
  char buf[BUFSIZ] = {0}, *infile = NULL, *outfile = NULL;

  // flags
  int segFaultFlag = 0;
  int catchFlag = 0;

  // option initializations                                                                 
  const struct option longopts[] = {
    { .name = "input",      .has_arg = required_argument,  .flag = NULL,  .val = INPUT     },
    { .name = "output",     .has_arg = required_argument,  .flag = NULL,  .val = OUTPUT    },
    { .name = "segfault",   .has_arg = no_argument,        .flag = NULL,  .val = SEGFAULT  },
    { .name = "catch",      .has_arg = no_argument,        .flag = NULL,  .val = CATCH     },
    { .name = "dump-core",  .has_arg = no_argument,        .flag = NULL,  .val = DUMP_CORE },
    { .name = 0,            .has_arg = 0,                  .flag = 0,     .val =  0 }
  };  

  // read options
  while(1) {
    if ((opt = getopt_long(argc, argv, "", longopts, &longindex)) == -1)
      break;
    switch(opt) {
    case 'i':
      if ((ifd = open(optarg, O_RDONLY)) == -1) {
	perror(optarg);
	if (infile != NULL) close(ifd);
	if (outfile != NULL) close(ofd);
	exit(2);
      }
      dup2(ifd, 0);
      infile = strdup(optarg);
      break;
    case 'o':
      if((ofd = open(optarg, O_WRONLY | O_APPEND | O_CREAT, 0600)) == -1) {
	perror(optarg);
	if (infile != NULL)  close(ifd);
        if (outfile != NULL) close(ofd);
	exit(3);
      }
      dup2(ifd, 1);
      outfile = strdup(optarg);
      break;
    case 's':
      segFaultFlag = 1;
      break;
    case 'c':
      catchFlag += 1;
      break;
    case 'd':
      catchFlag -= 1;
      break;
    case '?':
      if (infile != NULL)  close(ifd);
      if (outfile != NULL) close(ofd);
      exit(1);
    }    
  }
  
  //SIGSEGV Options
  if (catchFlag > 0) signal(SIGSEGV, sigsegv_handler);

  if (segFaultFlag) {
    if (infile != NULL)  close(ifd);
    if (outfile != NULL) close(ofd);
    doFault();
  }
  
  // read and write loop
  while ((size = read(ifd, buf, BUFSIZ))) {    
    if (size == -1) {
       perror(infile);
       if (infile != NULL)  close(ifd);
       if (outfile != NULL) close(ofd);
       exit(5);
    }
    if (write(ofd, buf, size) == -1) { 
      perror(outfile);
      if (infile != NULL)  close(ifd);
      if (outfile != NULL) close(ofd);
      exit(5);
    } 
  }
  
  //close and exit
  if (infile != NULL)  close(ifd);
  if (outfile != NULL) close(ofd);
  exit(0);
}

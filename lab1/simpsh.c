#include <getopt.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <sys/wait.h>
#include <limits.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <ctype.h>
#include <signal.h>

#define COMMAND 'a'
#define VERBOSE 'b'
#define WAIT 'c'
#define CHDIR 'd'
#define PIPE 'e'
#define CLOSE 'f'
#define ABORT 'g'
#define CATCH 'h'
#define IGNORE 'i'
#define DEFAULT 'j'
#define PAUSE 'k'
#define PROFILE 'l'
#define COMMAND_KEYWORD "--command"
#define FILE_PERMISSIONS (S_IWGRP | S_IWOTH)
#define BACK_DIRECTORY ".."
#define NUM_OPTS 14
#define GET_TIME 'm'

// for use by wait
struct thread_info { pid_t pid; int start_index; int end_index; int profile;};
// segfault handler for abort
void sig_handler(int sig);
// prints error message.  set opstr = "" for int error.
static inline void opt_error(int optint, const char* optstr, int* failed_opt); 
// output completed thread info
// prints option & corresponding argument(s) up to option w/ --
static inline void print_command(int start_index, int end_index, char** argv);
static inline void print_option(int option_index, struct option* longopts, char** argv, int argc);
static inline void print_data(const char* command,
			      struct timeval ru_utime, struct timeval utime, 
			      struct timeval ru_stime, struct timeval stime);
// converts a string of integers into the relevant sum
static inline int string_to_int(char* string);

int main(int argc, char** argv) {
  int longindex = 1, noptind, num_fd = 0, pipefd[2], fds[1024] = { [0 ... 1023 ] = -1 }; 
  int fd, inidx, outidx, erridx, access_type = 0,  verbose_flag = 0, sig;
  int ctr, boost, failed_opt = 0, buffer_len = 0, max_return_value = 0, max_signal_value = 0;
  char** buffer; long option; size_t flags = 0; int n_threads = 0, ret;
  int inmode, outmode, errmode, profile_flag=0, flag_flag=0, test_flag=0; 
  struct timeval prev_utime, prev_stime;
  struct thread_info* threads = NULL; struct rusage usage;
  umask(FILE_PERMISSIONS);
  static struct option longopts[] = {
    ///////////////////////////////////// PHASE A OPTIONS /////////////////////////////////////
    { .name = "rdonly",     .has_arg = required_argument,  .flag = NULL,  .val = O_RDONLY    },
    { .name = "wronly",     .has_arg = required_argument,  .flag = NULL,  .val = O_WRONLY    },
    { .name = "command",    .has_arg = required_argument,  .flag = NULL,  .val = COMMAND     },
    { .name = "verbose",    .has_arg = no_argument,        .flag = NULL,  .val = VERBOSE     },
    ///////////////////////////////////// PHASE B OPTIONS /////////////////////////////////////
    { .name = "wait",       .has_arg = no_argument,        .flag = NULL,  .val = WAIT        },
    { .name = "pipe",       .has_arg = no_argument,        .flag = NULL,  .val = PIPE        },
    { .name = "pause",      .has_arg = no_argument,        .flag = NULL,  .val = PAUSE       },
    { .name = "abort",      .has_arg = no_argument,        .flag = NULL,  .val = ABORT       },
    { .name = "catch",      .has_arg = required_argument,  .flag = NULL,  .val = CATCH       },
    { .name = "ignore",     .has_arg = required_argument,  .flag = NULL,  .val = IGNORE      },
    { .name = "default",    .has_arg = required_argument,  .flag = NULL,  .val = DEFAULT     },
    { .name = "close",      .has_arg = required_argument,  .flag = NULL,  .val = CLOSE       },
    { .name = "rdwr",       .has_arg = required_argument,  .flag = NULL,  .val = O_RDWR      }, 
    { .name = "chdir",      .has_arg = required_argument,  .flag = NULL,  .val = CHDIR       },
    ///////////////////////////////////// PHASE C OPTIONS /////////////////////////////////////
    { .name = "profile",    .has_arg = no_argument,        .flag = NULL,  .val = PROFILE     },
    ///////////////////////////////////// FLAGS & ZEROES //////////////////////////////////////
    { .name = "append",     .has_arg = no_argument,        .flag = NULL,  .val = O_APPEND    },
    { .name = "cloexec",    .has_arg = no_argument,        .flag = NULL,  .val = O_CLOEXEC   },
    { .name = "creat",      .has_arg = no_argument,        .flag = NULL,  .val = O_CREAT     },
    { .name = "directory",  .has_arg = no_argument,        .flag = NULL,  .val = O_DIRECTORY },
    { .name = "dsync",      .has_arg = no_argument,        .flag = NULL,  .val = O_DSYNC     },
    { .name = "excl",       .has_arg = no_argument,        .flag = NULL,  .val = O_EXCL      },
    { .name = "nofollow",   .has_arg = no_argument,        .flag = NULL,  .val = O_NOFOLLOW  },
    { .name = "nonblock",   .has_arg = no_argument,        .flag = NULL,  .val = O_NONBLOCK  },
    //    { .name = "rsync",      .has_arg = no_argument,        .flag = NULL,  .val = O_RSYNC     },
    { .name = "sync",       .has_arg = no_argument,        .flag = NULL,  .val = O_SYNC      },
    { .name = "trunc",      .has_arg = no_argument,        .flag = NULL,  .val = O_TRUNC     },
    //////////////////////////////////////// UTILITY //////////////////////////////////////////     
    { .name = "time",       .has_arg = no_argument,        .flag = NULL,  .val = GET_TIME    },
    { .name = 0,            .has_arg = 0,                  .flag = 0,     .val = 0           }
  };

  opterr = 0;
  while (1) {
    if ((option = getopt_long(argc, argv, "", longopts, &longindex)) == -1)
      break;
    
    // print commands
    if (option != '?' && verbose_flag) 
      print_option(optarg == NULL ? optind - 1 : optind - 2, longopts, argv, argc);
    fflush(NULL);

    // record time before executing option
    if (profile_flag && getrusage(RUSAGE_SELF, &usage) != -1) {
	prev_utime = usage.ru_utime;
	prev_stime = usage.ru_stime;
  }

    // deal with all options
    switch(option) {

    case O_RDONLY: case O_WRONLY: case O_RDWR:
      // set access type
      if (option == O_RDWR) access_type = R_OK;
      else if (option == O_WRONLY) access_type = W_OK;
      else if (option == O_RDONLY) access_type = (R_OK | W_OK);
      if (optarg[0] == '-' && optarg[1] == '-') {
	optind -=1;
	failed_opt = 1;
	fprintf(stderr, "Nol argument for option --%s\n", longopts[longindex].name);
	break;
      }
      
      flags |= option;
      if (access(optarg, access_type) == 0) {
	if ((fd = open(optarg, flags)) == -1)
	  opt_error(0, optarg, &failed_opt);
      }
      else if((access(optarg, F_OK)) != 0 && ((flags & O_CREAT) != 0)) {
	if ((fd = open(optarg, flags, 0666)) == -1)
	  opt_error(0, optarg, &failed_opt);
      }
      else opt_error(0, optarg, &failed_opt);
      // variable cleanup
      fds[num_fd++] = fd;
      flags = 0;
      access_type = 0;
      break;

      // FLAGS
    case O_CREAT:  case O_APPEND:   case O_CLOEXEC:    case O_DIRECTORY: case O_DSYNC: 
    case O_TRUNC:  case O_EXCL:     case O_NOFOLLOW:   case O_NONBLOCK:  case O_SYNC: 
      //ase O_RSYNC:
      flags |= option;
      flag_flag = 1;
      break;

    case PIPE:
      if (pipe(pipefd) == -1)
        opt_error(0, longopts[longindex].name, &failed_opt);
      fds[num_fd++] = pipefd[0];
      fds[num_fd++] = pipefd[1];
      break;

    case COMMAND: 
      // find command option indeces
      noptind = argc;
      for (ctr = optind-1; ctr < argc; ctr++) 
	if (argv[ctr][0] == '-' && argv[ctr][1] == '-') { noptind = ctr; break; }
      
      //not enough args for command
      if (noptind <= optind + 2 ) { opt_error(0, COMMAND_KEYWORD, &failed_opt); break; }
 
     // check file fds
      inidx = string_to_int(optarg);
      outidx = string_to_int(argv[optind++]);
      erridx = string_to_int(argv[optind++]);

      inmode = (fcntl(fds[inidx], F_GETFL) & O_ACCMODE);
      outmode = (fcntl(fds[outidx], F_GETFL) & O_ACCMODE);
      errmode = (fcntl(fds[erridx], F_GETFL) & O_ACCMODE);

      if ((inidx == -1) || (fcntl(fds[inidx], F_GETFD) == -1) || 
	  ((inmode != O_RDONLY) && (inmode != O_RDWR))) {
	opt_error(inidx, "", &failed_opt); break; }
      if ((outidx == -1) || (fcntl(fds[outidx], F_GETFD) == -1) || 
	  ((outmode != O_WRONLY) && (outmode != O_RDWR))) {
	opt_error(outidx, "", &failed_opt); break; }
      if ((erridx == -1) || (fcntl(fds[erridx], F_GETFD) == -1) || 
	  ((errmode != O_WRONLY) && (errmode != O_RDWR))) {
	opt_error(erridx, "", &failed_opt); break; }

      // set range
      boost = optind;
      optind = noptind;

      if ((sig = fork()) > 0) {
	threads = realloc(threads, (n_threads+1) * sizeof(struct thread_info));
	threads[n_threads++] = (struct thread_info) { sig, boost, noptind, profile_flag };
	break;      
      }

      // load ports and buffer command
      dup2(fds[inidx], 0);
      dup2(fds[outidx], 1);
      dup2(fds[erridx], 2);
      buffer = (char**) malloc((buffer_len = (noptind-boost+2)*sizeof(char*)));
      for (ctr = 0; ctr+boost < noptind; ctr++) 
	buffer[ctr] = argv[ctr+boost];
      buffer[ctr] = NULL;
      for (ctr = 0; ctr < num_fd; ctr++) close(fds[ctr]);

      // run command
      execvp(buffer[0], buffer);
      return 1;

    case WAIT:
        // determine exit status
      while ((ret = wait(&sig)) != -1) {
	if (WIFEXITED(sig)) {
	  fprintf(stdout, "exit %i ", WEXITSTATUS(sig));
	  if (WEXITSTATUS(sig) > max_return_value)
	    max_return_value = WEXITSTATUS(sig);
	}
	if (WIFSIGNALED(sig)) {
	  fprintf(stdout,"signal %i ", WTERMSIG(sig));
	  if (WTERMSIG(sig) > max_signal_value)
	    max_signal_value = WTERMSIG(sig);	  
	}
	for (ctr = 0; ctr < n_threads; ctr++) {
	  if (threads[ctr].pid == ret) {
	    print_command(threads[ctr].start_index, threads[ctr].end_index, argv);
	    break;
	  }
	}
      }
      //print time values if --profile
      if (profile_flag && threads != NULL && threads[ctr].profile) {	  
	if (getrusage(RUSAGE_CHILDREN, &usage) == -1) 
	  opt_error(0, "Child", &failed_opt);
	else 
	  print_data("children", 
		     usage.ru_utime, (struct timeval) {0, 0}, 
		     usage.ru_stime, (struct timeval) {0, 0});
      }

      fflush(NULL);
      free(threads);
      threads = NULL;
      break;

    case CHDIR:
      if (chdir(optarg) == -1) opt_error(0, optarg, &failed_opt);
      break;
      
    case CLOSE:
      if ((sig = string_to_int(optarg)) == -1 || sig >= num_fd || close(fds[sig]) == -1)
	opt_error(0, optarg, &failed_opt);
      fds[sig] = -1;
      break;

    case VERBOSE:
      verbose_flag = 1;
      break;

    case PROFILE:
      profile_flag = 1;
      flag_flag = 1;
      break;

    case ABORT:
      free(threads);
      threads = NULL;
      raise(SIGSEGV);
      break;

    case CATCH:
      if ((sig = string_to_int(optarg)) == -1 || signal(sig, sig_handler) == SIG_ERR) 
	opt_error(sig, "", &failed_opt);
      break;

    case IGNORE:
      if ((sig = string_to_int(optarg)) == -1 || signal(sig, SIG_IGN) == SIG_ERR)
	opt_error(*optarg, "", &failed_opt);
      break;

    case DEFAULT:
      if ((sig = string_to_int(optarg)) == -1 || signal(sig, SIG_DFL) == SIG_ERR)
	opt_error(*optarg, "", &failed_opt);
      break;

    case PAUSE:
      pause();
      opt_error(0, longopts[longindex].name, &failed_opt);
      break;

    case GET_TIME:
      test_flag = 1;
      break;

    case '?': default:
      // no argument for option with required argument
      if (longindex != -1) {
	fprintf(stderr, "No parameter for option \"%s\".\n", longopts[longindex].name);
	failed_opt = 1;
    }
      // bad option
      else {
	fprintf(stderr, "Bad Option: %s\n", argv[optind-1]); 
	for (ctr = 0; ctr < num_fd; ctr++) close(fds[ctr]);
	free(threads);
	exit(1);
      }
    }
    fflush(NULL);

    // get current time && subtract previous for output
    if  (option != -1 && profile_flag && !flag_flag) {
      getrusage(RUSAGE_SELF, &usage);
      print_data(longopts[longindex].name, usage.ru_utime, prev_utime, usage.ru_stime, prev_stime);
    }  
    flag_flag = 0;
  }

  fflush(NULL);
  for (ctr = 0; ctr < num_fd; ctr++) close(fds[ctr]);
  free(threads);
  
  if (test_flag) {
    getrusage(RUSAGE_SELF, &usage);
    fprintf(stdout, "TOTAL TIME: %.5fs| USER TIME: %.5fs| SYS TIME: %.5fs\n",
	   (usage.ru_utime.tv_sec) + (usage.ru_utime.tv_usec)/1000000.0
	    + (usage.ru_stime.tv_sec) + (usage.ru_stime.tv_usec)/1000000.0,
	    (usage.ru_utime.tv_sec) + (usage.ru_utime.tv_usec)/1000000.0,
	    (usage.ru_stime.tv_sec) + (usage.ru_stime.tv_usec)/1000000.0);
  }

  if (max_signal_value != 0) {
    signal(max_signal_value, SIG_DFL);
    raise(max_signal_value); 
  }
  exit((max_return_value!= 0) ? max_return_value : failed_opt);
 
}


///////////////////////////////////////////////////////////////////////////////////////////         
//////////////////////////////// UTILITY FUNCTIONS ////////////////////////////////////////       
///////////////////////////////////////////////////////////////////////////////////////////         

void sig_handler(int sig) {
  fprintf(stderr, "%i caught\n", sig);
  _exit(sig);
}

static inline void print_command(int start_index, int end_index, char** argv) {
  int ctr;
  for (ctr = start_index; ctr < end_index; ctr++) {
    fprintf (stdout, "%s ", argv[ctr]);
  }
  fprintf(stdout, "\n");
  fflush(NULL);
}

static inline void print_data(const char* command,
                              struct timeval ru_utime, struct timeval utime, 
			      struct timeval ru_stime, struct timeval stime) {
  fprintf(stdout, "%-12sUSER CPU TIME: %.3fs", command, 
	  (ru_utime.tv_sec - utime.tv_sec) + (ru_utime.tv_usec - utime.tv_usec)/1000000.0);
  fprintf(stdout, "%-12sSYSTEM CPU TIME: %.3fs\n", "",
	  (ru_stime.tv_sec - utime.tv_sec) + (ru_stime.tv_usec - stime.tv_usec)/1000000.0);
}

//far back as command, far forward as --
static inline void print_option(int opt_index, struct option* longopts, char** argv, int argc) {
  int start, end, ctr, flag=0;

  // determine command start index in argv
  for (start =  opt_index-1; (start > 0); start--) {
    for (ctr = 0; ctr < NUM_OPTS; ctr++)
      if (argv[start][0] != '-' || strcmp(longopts[ctr].name, argv[start]+2) == 0)
        { flag = 1; break; }
    if (flag) { start++; break; }
  }

  // determine command end index in argv
  for (end = opt_index+1; end < argc; end++)
    if (argv[end][0] == '-' && argv[end][1] == '-')
      break;

  // print
  ctr = start;
  while (ctr < end)
    fprintf(stdout, "%s ", argv[ctr++]);
  fprintf(stdout, "\n");
  fflush(NULL);
}

static inline void opt_error(int optint, const char* optstr, int* failed_opt)
{
  if (strcmp(optstr, COMMAND_KEYWORD) == 0)
    fprintf(stderr, "Missing argument for %s\n", COMMAND_KEYWORD);
  else if (strcmp(optstr, "") ==0)
    fprintf(stderr, "%s: %i\n", strerror(errno), optint);
  else
    fprintf(stderr, "%s: %s\n", strerror(errno), optstr);
  *failed_opt = 1;
  fflush(stderr);
}

static inline int string_to_int(char* string) {
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



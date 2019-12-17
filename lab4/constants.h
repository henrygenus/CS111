#ifndef constants_h
#define constants_h

#include <getopt.h>
#include <unistd.h>
#include <stdbool.h>

#define SCALE 'a'
#define PERIOD 'b'
#define STOP 'c'
#define START 'd'
#define LOG 'e'
#define OFF 'f'
#define ID 'g'
#define HOST 'h'
#define RUNTIME_ERROR 2
#define SYS_ERROR _system_call_error_(NULL)

#define PERMISSIONS 0666
#define LOGFILE_FLAGS O_APPEND | O_CREAT | O_RDWR

bool subcmp(const char *string1, const char *string2, unsigned int len);
int _system_call_error_(char *arg);
#endif /* constants_h */

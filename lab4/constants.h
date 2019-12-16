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

#define PERMISSIONS 0666
#define LOGFILE_FLAGS O_APPEND | O_CREAT | O_RDWR

bool subcmp(const char *string1, const char *string2, unsigned int len);

#endif /* constants_h */

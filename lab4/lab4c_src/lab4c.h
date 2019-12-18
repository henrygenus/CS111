#ifndef lab4c_tcp_h
#define lab4c_tcp_h

#include "../constants.h"
#include <stdbool.h>
#include <openssl/ssl.h>
#include <openssl/conf.h>

#define READ 0
#define WRITE 1
#define DEVICE "./lab4b"

#define DEBUG_FLAG

#ifdef DEBUG_FLAG
#define DEBUG_PRINT 1
#else
#define DEBUG_PRINT 0
#endif

typedef struct port_data {
    SSL *ssl_client;
    int *socket;
} port;

typedef struct tcp_data {
    char id[10];
    char *host;
    char *logfile;
    int portno;
} tcp;
// id is the 9 digit unique identifier
// host is the name or address of the host
// log is the file descriptor associated with the log
// portno is the port number for sending and receiving data


// swap parent and child read pipes st p_read <—> c_write & vice versa
extern void exchange_pipes(int parent_pipe[2], int child_pipe[2]);
int srv_connect(tcp tcp, bool tls_flag, int *server, SSL_CTX *context, SSL *client);
int close_ssl(SSL *ssl_client);
// processses command line arguments
extern int process_command_line(int argc, char **argv, tcp *tcp, char **args);
// checks options initialized by command line (pipes initialized here)
extern int check_options(tcp *tcp);
// loop to get input from infd and write to outfd
void process_output(port *from, port *to, int device_flag);
// write or tls write depending on port type (used for initial write)
extern int do_write(port *port, char *string, int size);

#endif /* lab4c_tcp_h */

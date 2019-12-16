#ifndef lab4c_tcp_h
#define lab4c_tcp_h

#include "../constants.h"
#include <stdbool.h>
#include <openssl/ssl.h>
#include <openssl/conf.h>

#define READ 0
#define WRITE 1
#define DEVICE "./lab4b"

typedef struct port_data {
    SSL *ssl_client;
    int *socket;
} port;

typedef struct tcp_data {
    char id[10];
    char *host;
    char* logfile;
    int portno;
} tcp;
// id is the 9 digit unique identifier
// host is the name or address of the host
// log is the file descriptor associated with the log
// portno is the port number for sending and receiving data

// swap parent and child read pipes st p_read <—> c_write & vice versa
extern void exchange_pipes(int parent_pipe[2], int child_pipe[2]);
// connect via tcp to server, return fd
int client_connect(char * host_name, unsigned int port);
// processses command line arguments
 extern int process_command_line(int argc, char **argv, tcp *tcp, char **args);
// checks options initialized by command line (pipes initialized here)
extern void check_options(tcp *tcp, int parent_pipe[2], int child_pipe[2]);
// check condition for stopping output reading (varies if server vs device)
bool last_entry(pid_t device, char *buffer);
// loop to get input from infd and write to outfd
void process_output(port *from, port *to, int device_flag);
// create a tls context
SSL_CTX *ssl_init(void);
// bind a socket to a tls context
SSL *attach_ssl_to_socket(int socket, SSL_CTX *context);
// read or tls read depending on port type
extern int do_read(port *port, char *string, int size);
// write or tls write depending on port type
extern int do_write(port *port, char *string, int size);
#endif /* lab4c_tcp_h */

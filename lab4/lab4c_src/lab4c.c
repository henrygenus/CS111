#include "lab4c.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>


static struct option longopts[] = {
    { "log",    required_argument, NULL, LOG    },
    { "scale",  required_argument, NULL, SCALE  },
    { "period", required_argument, NULL, PERIOD },
    { "id",     required_argument, NULL, ID     },
    { "host",   required_argument, NULL, HOST   },
    { 0, 0, 0, 0 }
};

/*
	* server connection functions
	*/
// connect via tcp to server, return fd
int client_connect(char * host_name, unsigned int port);
// create a tls context
SSL_CTX *ssl_init(void);
// bind a socket to a tls context
SSL *attach_ssl_to_socket(int socket, SSL_CTX *context);


// ////////////////////////////////////////////////////////////////////////
// ////////////////////// FUNCTION IMPLEMENTATIONS ////////////////////////
// ////////////////////////////////////////////////////////////////////////

int process_command_line(int argc, char** argv, tcp *tcp, char **args) {
    int longindex = 0, opt = 0, ctr;
    int max_ind = 0;
    char buffer[BUFSIZ];
    while (true) {
        if ((opt = getopt_long(argc, argv, "", longopts, &longindex)) == -1) break;
        // interpret arguments
        switch (opt) {
            case ID:
																if (strcpy(tcp->id, optarg) == NULL) return SYS_ERROR;
                break;
            case HOST:
																if ((tcp->host = strdup(optarg)) == NULL) return SYS_ERROR;
                break;
            case LOG:
																if ((tcp->logfile = strdup(optarg)) == NULL) return SYS_ERROR;
            case SCALE: case PERIOD:
                if (sprintf(buffer, "%s", argv[optind-1]) == -1) return SYS_ERROR;
                args[++max_ind] = strdup(buffer);
                break;
												default: return -1;
        }
    }
    args[++max_ind] = NULL;
                               
    for(ctr = 1; ctr < argc; ctr++) {
        if (strlen(argv[ctr]) > 1 && argv[ctr][0] == '-' && argv[ctr][1] == '-')
            continue;
        else if (tcp->portno != -1) {
            fprintf(stderr, "Multiple port numbers given");
												return -1;
        }
        else tcp->portno = atoi(argv[ctr]);
    }
				if (DEBUG_PRINT) {
								for (ctr = 0; ctr <= max_ind; ctr++) fprintf(stdout, "%s ", args[ctr]);
								fprintf(stdout, "\n");
								fflush(NULL);
				}
    return max_ind + 1;
}

void process_output(port *from, port *to, int device_flag) {
    char buffer[BUFSIZ] = {0};
    int count = BUFSIZ;
    // read command from server
    do {
        if((count = do_read(from, buffer, BUFSIZ)) > 0) {
            // write to device
            do_write(to, buffer, count);
            // output for error checking
            if (DEBUG_PRINT) write(2, buffer, count);
        }
    } while (! last_entry(device_flag, buffer));
}

int check_options(tcp *tcp) {
    if (strlen(tcp->id) < 9) fprintf(stderr, "Bad ID number\n");
    else if (strlen(tcp->host) <= 0) fprintf(stderr, "Bad host name\n");
    else if (tcp->logfile == NULL || strlen(tcp->logfile) <= 0)
								fprintf(stderr, "Bad logfile name\n");
    else if(tcp->portno <= 0) fprintf(stderr, "Bad port numbern\n");
				return 0;
}

// ////////////////////////////////////////////////////////////////////////
// //////////////////////// SERVER FUNCTIONS //////////////////////////////
// ////////////////////////////////////////////////////////////////////////

int srv_connect(tcp tcp, bool tls_flag, int *server, SSL_CTX *context, SSL *client) {
				if ((*server = client_connect(tcp.host, tcp.portno)) == -1) return -1;
    
    // initialize tls connection (optional)
    if (tls_flag) {
								if ((context = ssl_init()) == NULL) return SYS_ERROR;
        if ((client = attach_ssl_to_socket(*server, context)) == NULL)
												return SYS_ERROR;
    }
				return 0;
}

int client_connect(char *host_name, unsigned int port) {
    //encode the ip address and the port for the remote
    struct sockaddr_in serv_addr;
				int sockfd;
				if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) return SYS_ERROR;
    struct hostent *server = NULL;
				if((server = gethostbyname(host_name)) == NULL) return SYS_ERROR;
   
    // convert host_name to IP addr
    memset(&serv_addr, 0, sizeof(struct sockaddr_in));
    serv_addr.sin_family = AF_INET; //address is Ipv4
    memcpy(&serv_addr.sin_addr.s_addr, server->h_addr, server->h_length);
   
    //copy ip address from server to serv_addr
    serv_addr.sin_port = htons(port); //setup the port
    
    //initiate the connection to server
    if (connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) == -1)
								return SYS_ERROR;
				return sockfd;
}

SSL_CTX *ssl_init(void) {
			SSL_CTX * newContext = NULL;
			SSL_library_init();
			//Initialize the error message
			SSL_load_error_strings();
			OpenSSL_add_all_algorithms();
			//TLS version: v1, one context per server.
			newContext = SSL_CTX_new(TLSv1_client_method());
			return newContext;
}

SSL *attach_ssl_to_socket(int socket, SSL_CTX *context) {
				SSL *sslClient = SSL_new(context);
				SSL_set_fd(sslClient, socket);
				SSL_connect(sslClient);
				return sslClient;
}

// ////////////////////////////////////////////////////////////////////////
// //////////////////////// INLINE FUNCTIONS //////////////////////////////
// ////////////////////////////////////////////////////////////////////////

int do_read(port *port, char *string, int size) {
    return(port->ssl_client == NULL ? (int)read(*port->socket, string, size)
           : SSL_read(port->ssl_client, string, size));
}
            
int do_write(port *port, char *string, int size) {
    return (port->ssl_client == NULL ? (int)write(*port->socket, string, size)
            : SSL_write(port->ssl_client, string, size));
}

bool last_entry(pid_t device, char *buffer) {
    int ctr = 0;
    while (buffer[ctr] != ' ') ctr++;
    return (device ? subcmp(&buffer[++ctr], "SHUTDOWN\n", 9)
												: subcmp(buffer, "OFF\n", 4));
}

void exchange_pipes(int parent_pipe[2], int child_pipe[2]) {
    int temp;
    temp = parent_pipe[READ];
    parent_pipe[READ] = child_pipe[READ];
    child_pipe[READ] = temp;
}

bool subcmp(const char *string1, const char *string2, unsigned int len){
    unsigned int ctr;
    if (strlen(string1) < len || strlen(string2) < len) return false;
    for(ctr = 0; ctr < len; ctr++)
        if (string1[ctr] != string2[ctr]) return false;
    return true;
}

int _system_call_error_(char *arg) {
				perror(arg);
				return -1;
}

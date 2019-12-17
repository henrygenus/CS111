#include "lab4c.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h> // wait
#include <sys/wait.h> // wait


int main(int argc, char **argv) {
    // utility variables
    tcp tcp = {{0}, NULL, NULL, -1};
    int parent_pipe[2], child_pipe[2], wstatus = 0, server, ret = 0;
    pid_t device_pid, dev_to_srv, srv_to_dev, pid;
    char *args[BUFSIZ] = {0}; args[0] = strdup(DEVICE);
    SSL_CTX *context = NULL;
    SSL *ssl_client = NULL;
    port in = (port){NULL, NULL}, out = (port){NULL, NULL};

    // tls dependent variables
    size_t ctr = strlen(argv[0]);
    while(argv[0][ctr] != '/' && ctr != 0) ctr--;
    int tls_flag = (strcmp(&argv[0][++ctr], "lab4c_tls") == 0);
    if (DEBUG_PRINT) fprintf(stderr, "%i", tls_flag);
        
    // initialize connection with server
    process_command_line(argc, argv, &tcp, args);
    check_options(&tcp, parent_pipe, child_pipe);
    server = client_connect(tcp.host, tcp.portno);
    
    // initialize tls connection (optional)
    if (tls_flag) {
        if ((context = ssl_init()) == NULL) {
            fprintf(stderr, "%s\n", strerror(errno));
            exit(2);
        }
        ssl_client = attach_ssl_to_socket(server, context);
    }
    
    if (DEBUG_PRINT) fprintf(stderr, "Parent Process PID: %i\n", getpid());
    
    // ////////////////////// child: device /////////////////////
    // runs lab4b program with writes to log
    if ((device_pid = fork()) == 0) {
        if (DEBUG_PRINT) fprintf(stderr, "DEV PREPPING (%i)\n", getpid());
        // close other pipes
        close(parent_pipe[WRITE]);
        close(parent_pipe[READ]);
        // setup environment
        dup2(child_pipe[READ], READ);
        dup2(child_pipe[WRITE], WRITE);
        if (DEBUG_PRINT) fprintf(stderr, "DEV STARTING\n");
        // activate device
        execvp(DEVICE, args);
     
        // if we reach here, there was an error
								perror(NULL);
        exit(1);
    }
    // fork failure
    else if (device_pid == -1) {
        fprintf(stderr, "%s\n", strerror(errno));
        exit(1);
    }
    
    // ////////////////// parent: moderator ///////////////////
    // mediates communication between device and server
    // split in 2 for contact between device & server
    
    // server -> device
    if ((srv_to_dev = fork()) == 0) {
        if (DEBUG_PRINT) fprintf(stderr, "SRV->DEV PREPPING (%i)\n", getpid());
        // close other pipes
        close(parent_pipe[READ]);
        close(child_pipe[READ]);
        close(child_pipe[WRITE]);
        // setup environment
        in.socket = &server;
        in.ssl_client = ssl_client;
        out.socket = &parent_pipe[WRITE];
        out.ssl_client = NULL;
        // loop read
        if (DEBUG_PRINT) fprintf(stderr, "SRV->DEV STARTING\n");
        process_output(&in, &out, 0);
        if (DEBUG_PRINT) fprintf(stderr, "SRV->DEV FINISHED\n");
        // cleanup
        close(server);
        close(parent_pipe[WRITE]);
        exit(0);
    }
    // fork failure
    else if (srv_to_dev == -1) {
        fprintf(stderr, "%s\n", strerror(errno));
        exit(1);
    }

    // device -> server
    else if ((dev_to_srv = fork()) == 0) {
        if (DEBUG_PRINT) fprintf(stderr, "DEV->SRV PREPPING (%i)\n", getpid());
        // close other pipes
        close(parent_pipe[WRITE]);
        close(child_pipe[READ]);
        close(child_pipe[WRITE]);
        // setup environment
        in.socket = &parent_pipe[READ];
        in.ssl_client = NULL;
        out.socket = &server;
        out.ssl_client = ssl_client;
        // initiate command delivery
        if (DEBUG_PRINT) fprintf(stderr, "DEV->SRV STARTING\n");
        do_write(&out, "ID=304965058\n", 13);
        if (DEBUG_PRINT) fprintf(stderr, "ID=%s\n", tcp.id);
        // loop read
        process_output(&in, &out, device_pid);
        if (DEBUG_PRINT) fprintf(stderr, "DEV->SRV FINISHING\n");
        // cleanup
        close(server);
        close(parent_pipe[READ]);
        exit(0);

    }
    // fork failure
    else if (dev_to_srv == -1) {
        fprintf(stderr, "%s\n", strerror(errno));
        exit(1);
    }
    
    // check return statuses
    while((pid = wait(&wstatus)) != -1) {
        if (DEBUG_PRINT)
												fprintf(stderr, "%i exit status %d\n", pid, WEXITSTATUS(wstatus));
        if (WEXITSTATUS(wstatus) > ret) ret = WEXITSTATUS(wstatus);
    }

    // close tls (if one was used)
    if (tls_flag) {
        SSL_shutdown(ssl_client);
        SSL_free(ssl_client);
    }
    
    exit(ret);
}

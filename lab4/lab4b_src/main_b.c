#include "lab4b.h"
#include <poll.h> // for poll
#include <stdlib.h> // exit
#include <errno.h> //errno
#include <string.h> //strerror
#include <stdio.h> // BUFSIZ, print fcns


int main(int argc, char** argv) {
    // utility variables
    time_t then = 0; char time_string[BUFSIZ];
    device device = {0, 0, 1, 1};
    flags flags = {true, true, false, false};
    struct pollfd *pollfds = &((struct pollfd) {0, POLLIN, 0});
    
    // check initial parameters
    if(process_command_line(argc, argv, &flags, &device) == -1) exit(1);

    // initialize devices
    if(initialize_sensors(&device) == -1) exit(2);
    
       
    // loop to print, get commands, check button
    while(flags.run_flag) {
        // poll for input
        if (flags.report_flag)
            try_to_report(flags, &device, time_string, &then);
                
        // poll and optionally get command
        if (poll(pollfds, 1, 5) && pollfds->revents & POLLIN)
            process_command(&flags, &device);
      
        // check button for finality
        if (check_button(device.button))
            flags.run_flag = false;
    }
    
    // process all remaining flags
    while(poll(pollfds, 1, 5) && pollfds->revents & POLLIN) {
        if(get_time(time_string, &then) == -1) exit(1);
        else if(process_command(&flags, &device) == false)
            break;
    }
    // print exit command
    if (fprintf(stdout, "%s SHUTDOWN\n", time_string) == -1) {
        fprintf(stderr, "%s\n", strerror(errno));
        exit(1);
    }
    if (flags.log_flag && dprintf(device.log, "%s SHUTDOWN\n", time_string) == -1) {
        fprintf(stderr, "%s\n", strerror(errno));
        exit(1);
    }

    // close and exit
    close_sensors(&device);
    exit(0);
}

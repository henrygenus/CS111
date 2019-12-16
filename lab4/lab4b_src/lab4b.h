#ifndef lab4b_h
#define lab4b_h

#include "../constants.h"
#include <time.h>

// #include "debug.h"

#include <mraa/gpio.h>
#include <mraa/aio.h>

// struct to allow flags to be passed neatly
typedef struct flag_struct {
    bool run_flag;
    bool report_flag;
    bool log_flag;
    bool celsius_flag;
} flags;
// run_flag denotes whether the program should continue to run
// report_flag denotes whether the program should continue to output
// log_flag denotes whether we write to a log & —> must write commands
// celsius_flag denotes whether the temperature should be in celsius (default false)

// struct for visual clarity when passing common system details
typedef struct device_struct {
    mraa_gpio_context button;
    mraa_aio_context thermometer;
    int log;
    int period;
} device;
// thermometer denotes the port number of the thermometer device
// button denotes the port number of the button device
// log holds the location to write to (default = stdout)
// period determines how often writes should occur (default = 1)


// read from button for exit
extern int check_button(mraa_gpio_context button);
// call aio_init and gpio_init on appropriate devices
extern int initialize_sensors(device *device);
// call aio_close and gpio_close on appropriate devices
extern void close_sensors(device *device);
// gets the time and checks it against period for print
int get_time(char *time_string, time_t *now);
//// get time, check if it has been period, and report if it has
void try_to_report(flags flags, device *device, char *time_string, time_t *then);
// process command line arguments
int process_command_line(int argc, char **argv, flags *flags, device *device);
// process a command through the next '\n'
bool process_command(flags *flags, device *device);

#endif /* lab4b_h */

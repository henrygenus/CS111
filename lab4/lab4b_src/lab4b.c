#include "lab4b.h"
#include <time.h>
#include <math.h> // log
#include <string.h> // strerror, strcmp, strlen
#include <stdio.h> // print functions
#include <stdlib.h> // exit
#include <sys/stat.h> // file permissions
#include <fcntl.h> // open
#include <getopt.h> // getopt function
#include <unistd.h> // longopts structure
#include <errno.h> // errno


#define GPIO_INDEX 60
#define AIO_INDEX 1

static struct option longopts[] = {
    { "log",    required_argument, NULL, LOG    },
    { "scale",  required_argument, NULL, SCALE  },
    { "period", required_argument, NULL, PERIOD },
    { "id",     required_argument, NULL, ID     },
    { "host",   required_argument, NULL, HOST   },
    { 0, 0, 0, 0 }
};

// convert the temperature reading to celsius/fahrenheit
inline float convert_temperature_reading(int reading, int celsius_flag);
// get the current temperature
inline int get_temp(char* temp_string, mraa_aio_context thermometer, int celsius_flag);

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////// FUNCTION IMPLEMENTATIONS ///////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int check_button(mraa_gpio_context button) { return mraa_gpio_read(button); }

int initialize_sensors(device *device) {
    if (!(device->thermometer = mraa_aio_init(AIO_INDEX))) return SYS_ERROR;
				if (!(device->button = mraa_gpio_init(GPIO_INDEX))) return SYS_ERROR;
				mraa_gpio_dir(device->button, MRAA_GPIO_IN);
				return 0;
}

void close_sensors(device *device) {
    mraa_gpio_close(device->button);
    mraa_aio_close(device->thermometer);
}

int get_time(char *time_string, time_t *now) {
    struct timespec tspec;
				if (clock_gettime(CLOCK_REALTIME, &tspec) == -1) return SYS_ERROR;
				*now = tspec.tv_sec;
    struct tm ts = *localtime(now);
    if (sprintf(time_string, "%02d:%02d:%02d",
																ts.tm_hour, ts.tm_min, ts.tm_sec) == -1) return SYS_ERROR;
    //strftime(time_string, sizeof(time_string), "%H:%M:%S", &ts);
    if (strlen(time_string) != 8) {
								fprintf(stderr, "Invalid time string\n");
								return -1;
				}
    return 0;
}

int try_to_report(flags flags, device *device, char *time_string, time_t *then) {
    time_t now = 0; char temp_string[27];
    // check if it has been (period) seconds
    if(get_time(time_string, &now) == -1) return SYS_ERROR;
    if(now - *then >= device->period) {
        *then = now;
        get_temp(temp_string, device->thermometer, flags.celsius_flag);
        
        //do write
        if (flags.log_flag)
            if(dprintf(device->log, "%s %s\n", time_string, temp_string) == -1)
																return SYS_ERROR;
        if (fprintf(stdout, "%s %s\n", time_string, temp_string) == -1)
												return SYS_ERROR;
        fflush(stdout);
    }
				return 0;
}

int process_command_line(int argc, char** argv, flags *flags, device *device){
    int opt, longindex = 0;
    while (true) {
        if ((opt = getopt_long(argc, argv, "", longopts, &longindex)) == -1) break;
        switch(opt) {
            case SCALE: flags->celsius_flag = (!strcmp(optarg, "C")); break;
            case PERIOD: device->period = atoi(optarg); break;
            case LOG:
                flags->log_flag = true;
                if((device->log = open(optarg, LOGFILE_FLAGS, PERMISSIONS)) == -1)
																				return SYS_ERROR;
                break;
												default: return SYS_ERROR;
        }
    }
    return 0;
}

// string length overflow prevention is placed on the programmer
int process_command(flags *flags, device *device) {
    char buffer[BUFSIZ] = {0};
    char command[BUFSIZ] = {0};
    int ctr= 0, iter = 0;
    
    // get a command
    fgets(buffer, BUFSIZ, stdin);
    if(strlen(buffer) == 0) return false;
    
    // print command into log
    if (flags->log_flag && dprintf(device->log, "%s", buffer) == -1)
								return SYS_ERROR;
               
    // put each word into pointer with null byte instead of newline or space
    for(iter = 0; ctr < BUFSIZ; ctr++, iter++) {
        if(buffer[ctr] == '\n' || buffer[ctr] == '\0') break;
        command[iter] = buffer[ctr];
    } command[iter] = '\0';
    
    // deal with command
    if (strcmp(command, "OFF") == 0) flags->run_flag = false;
    else if (strcmp(command, "START") == 0) flags->report_flag = true;
    else if (strcmp(command, "STOP") == 0) flags->report_flag = false;
    else if (subcmp(command, "PERIOD=", 7)) device->period = atoi(&command[7]);
    else if (subcmp(command, "SCALE=", 6)) flags->celsius_flag = (command[6] == 'C');
    else if (subcmp(command, "LOG", 3)) ;
    //{ if(!flags->log_flag) if(fprintf(stdout, "%s", buffer) == -1) exit(1); }
				else { fprintf(stderr, "Bad Command: %s\n", command); return -1; }
    
    return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////// INLINE IMPLEMENTATIONS ////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool subcmp(const char *string1, const char *string2, unsigned int len) {
    unsigned int ctr;
    if (strlen(string1) < len || strlen(string2) < len) return false;
    for(ctr = 0; ctr < len; ctr++)
        if (string1[ctr] != string2[ctr]) return false;
    return true;
}

#define B 4275
#define R0 100000.0

float convert_temperature_reading(int reading, int celsius_flag)
{
    float R = 1023.0/((float) reading) - 1.0;
    R = R0*R;
    float C = 1.0/(log(R/R0)/B + 1/298.15) - 273.15;
    return celsius_flag ? C : (C * 9)/5 + 32;
}

// get the current temperature
int get_temp(char* temp_string, mraa_aio_context thermometer, int celsius_flag) {
    if(sprintf(temp_string, "%0.1f", convert_temperature_reading(mraa_aio_read(thermometer), celsius_flag)) == -1)
								return SYS_ERROR;
				return 0;
}

int _system_call_error_(char *arg) {
				perror(arg);
				return -1;
}

#ifndef DEBUG_H
#define DEBUG_H

// this is a dummy library with faux implementations of necessary mraa functions
// this is used for debugging without necessitation of a beaglebone

#define    MRAA_GPIO_IN    0

typedef int mraa_aio_context;
typedef int mraa_gpio_context;

int mraa_aio_init(int address);
int mraa_aio_read(mraa_aio_context c);
void mraa_aio_close(mraa_aio_context c);

int mraa_gpio_init(int address);
int mraa_gpio_read(mraa_aio_context c);
void mraa_gpio_dir(mraa_aio_context c, int type);
void mraa_gpio_close(mraa_aio_context c);

#endif /* lab4_h */

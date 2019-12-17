#include "debug.h"
#include <stdio.h>

int mraa_aio_init(int address) { return address + 1; }
int mraa_aio_read(mraa_aio_context c)    { return 650 - c + c; }
void mraa_aio_close(mraa_aio_context c)  { if (!c) printf(" ");}

int mraa_gpio_init(int address) { return address + 1; }
int mraa_gpio_read(mraa_aio_context c) {  return c - c;}
void mraa_gpio_dir(mraa_aio_context c, int type) { if (type || c) return; else printf(" "); }
void mraa_gpio_close(mraa_aio_context c)  { if (!c) printf(" ");}
int mraa_gpio_isr(mraa_gpio_context dev, mraa_gpio_edge_t edge, void(*fptr)(void *), void *args) {return 0;}

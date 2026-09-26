#include "pin_console.h"

#include <stdio.h>
#include <stdint.h>

void pin_console_begin(void)
{
    printf("PIN: 0");
    fflush(stdout);
}

void pin_console_update_digit(uint8_t digit)
{
    printf("\b%u", (unsigned)digit);
    fflush(stdout);
}

void pin_console_next_digit(void)
{
    printf(" 0");
    fflush(stdout);
}

void pin_console_end(void)
{
    printf("\n");
    fflush(stdout);
}
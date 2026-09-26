#ifndef PIN_CONSOLE_H_
#define PIN_CONSOLE_H_

#include <stdint.h>

void pin_console_begin(void);
void pin_console_update_digit(uint8_t digit);
void pin_console_next_digit(void);
void pin_console_end(void); // ?

#endif
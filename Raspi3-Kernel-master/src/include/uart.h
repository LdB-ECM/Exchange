#ifndef UART_H
#define UART_H
#include <stdint.h>

void uart_init();
void uart_send(char c);
char uart_getc();
void uart_puts(char *s);
void uart_hex(uint32_t d);
void uart_dump(void *ptr);

#endif

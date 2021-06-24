#ifndef UART_H
#define UART_H

void uart_init(void);
void uart_send_str(char* str);
char* uitoa_10(unsigned num, char* const str);

#endif  // UART_H
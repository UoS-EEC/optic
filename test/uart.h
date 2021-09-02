// Copyright (c) 2020-2021, University of Southampton.
// All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef TEST_UART_H_
#define TEST_UART_H_

void uart_init(void);
void uart_send_str(char* str);
char* uitoa_10(unsigned num, char* const str);

#endif  // TEST_UART_H_

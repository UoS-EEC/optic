#ifndef IC_H
#define IC_H

#include <stdint.h>

#define PERSISTENT __attribute__((section(".persistent")))

void adc12_init(void);
void comp_init(void);
void atom_func_start(void);
void atom_func_end(void);


// // configs
// #define AES_128_ENC      0
// #define AES_128_DEC      1
// #define ATOM_FUNC_NUM    2

#endif // IC_H
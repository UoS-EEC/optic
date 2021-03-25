#ifndef IC_H
#define IC_H

#include <stdint.h>
#include <stddef.h>

void atom_func_start(uint8_t func_id);
void atom_func_end(uint8_t func_id);


// configs
// aes
// #define AES_128_ENC      0
// #define AES_128_DEC      1
// #define ATOM_FUNC_NUM    2

// radio
#define RADIO            0
#define ATOM_FUNC_NUM    1

#endif // IC_H
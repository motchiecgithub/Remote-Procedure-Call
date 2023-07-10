#ifndef _UTILS_H_
#define _UTILS_H_
#include <stddef.h>
#include <stdint.h>
#include "rpc.h"
#define DATA1_SIZE 8
#define DATA2_LEN_SIZE 8

unsigned char *serialize(rpc_data *data);
rpc_data *decoding(unsigned char* buffer);
#endif
/*
    Reference: Beej's networking guide, Serialization - How to Pack Data
    https://beej.us/guide/bgnet/html/#serialization
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <netdb.h>
#include "utils.h"
#include <endian.h>

#define VOIDPTR 8

void packi64(unsigned char *buf, unsigned long long int i)
{
    // Beej's networking guide, Serialization - How to Pack Data
    *buf++ = i>>56; 
    *buf++ = i>>48;
    *buf++ = i>>40; 
    *buf++ = i>>32;
    *buf++ = i>>24; 
    *buf++ = i>>16;
    *buf++ = i>>8;  
    *buf++ = i;
}

long long int unpacki64(unsigned char *buf)
{
    // Beej's networking guide, Serialization - How to Pack Data
    unsigned long long int i2 = ((unsigned long long int)buf[0]<<56) |
                                ((unsigned long long int)buf[1]<<48) |
                                ((unsigned long long int)buf[2]<<40) |
                                ((unsigned long long int)buf[3]<<32) |
                                ((unsigned long long int)buf[4]<<24) |
                                ((unsigned long long int)buf[5]<<16) |
                                ((unsigned long long int)buf[6]<<8)  |
                                buf[7];
    long long int i;

    // change unsigned numbers to signed
    if (i2 <= 0x7fffffffffffffffu) { i = i2; }
    else { i = -1 -(long long int)(0xffffffffffffffffu - i2); }

    return i;
}

unsigned char *serialize(rpc_data* data){
    size_t len = DATA1_SIZE + DATA2_LEN_SIZE + (size_t)data->data2_len + VOIDPTR;
    unsigned char *byte_arr = malloc(len);
    packi64(byte_arr, data->data1);
    packi64(byte_arr + DATA1_SIZE, data->data2_len);
    memcpy(byte_arr + DATA1_SIZE + DATA2_LEN_SIZE, data->data2, data->data2_len);
    byte_arr[8 + 8 + data->data2_len] = '\0';
    return byte_arr;
}

rpc_data *decoding(unsigned char *buffer){
    rpc_data *data = malloc(sizeof(rpc_data));
    data->data1 = unpacki64(buffer);
    data->data2_len = unpacki64(buffer + DATA1_SIZE);
    if (data->data2_len == 0){
        data->data2 = NULL;
    } else {
        data->data2 = malloc(data->data2_len);
        memcpy(data->data2, buffer + 16, data->data2_len);
    }
    return data;
}


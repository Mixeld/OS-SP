#include <stdlib.h>
#include <string.h>
#include "common.h"

uint16_t message_calc_hash(const Message *msg)
{
    unsigned long hash;
    int aligned_len;
    int i;

    hash = 5381;

    hash = ((hash << 5) + hash) + msg->type;
    hash = ((hash << 5) + hash) + 0;  
    hash = ((hash << 5) + hash) + 0;  
    hash = ((hash << 5) + hash) + msg->size;

    aligned_len = msg->size + 1;
    for (i = 0; i < aligned_len; i++) {
        hash = ((hash << 5) + hash) + msg->data[i];
    }

    return (uint16_t)hash;
}

int message_check_hash(const Message *msg)
{
    Message tmp;

    memcpy(&tmp, msg, sizeof(Message));
    tmp.hash = 0;

    if (message_calc_hash(&tmp) == msg->hash) {
        return 0;   
    }

    return -1;      
}

void message_generate(Message *msg)
{
    int real_len;
    int aligned_len;
    int i;

    msg->type = (uint8_t)(rand() % 256);
    msg->size = (uint8_t)(rand() % 256);
    msg->hash = 0;

    real_len    = msg->size + 1;
    aligned_len = ((msg->size + 4) / 4) * 4;

    for (i = 0; i < aligned_len; i++) {
        if (i < real_len) {
            msg->data[i] = (unsigned char)(rand() % 256);
        } else {
            msg->data[i] = 0;
        }
    }

    msg->hash = message_calc_hash(msg);
}
#include "common.h"

unsigned short calculate_hash(message_t* msg) {
    unsigned short hash = 0;
    unsigned char* bytes = (unsigned char*)msg;
    
    // type(1) + hash(2) + size(1) + data(msg->size+1)
    int total_len = 1 + 2 + 1 + (msg->size + 1);
    
    // Защита от выхода за границы структуры
    if (total_len > 260) {
        total_len = 260;
    }
    
    for (int i = 0; i < total_len; i++) {
        // Пропускаем поле hash (индексы 1 и 2)
        if (i == 1 || i == 2) continue;
        
        hash ^= bytes[i];
        hash = (hash << 1) | (hash >> 15);
    }
    
    return hash;
}
#include "common.h"

void init_ring_buffer(ring_buffer_t* buf, int size){
    memset(buf, 0, sizeof(ring_buffer_t));
    for (int i = 0; i < size; i++){
        buf->buffer[i] = NULL;
    }
    buf->head = 0;
    buf->tail = 0;
    buf->added_count = 0;
    buf->removed_count = 0;
}

int get_queue_used(ring_buffer_t* buf){
    return buf->added_count - buf->removed_count;
}

int get_queue_free(ring_buffer_t* buf){
    return QUEUE_SIZE - get_queue_used(buf);
}

int enqueue_message(ring_buffer_t* buf, message_t* msg){
    if(get_queue_used(buf) >= QUEUE_SIZE){
        return -1;
    }
    buf->buffer[buf->tail] = msg;
    buf->tail = (buf->tail + 1) % QUEUE_SIZE;
    buf->added_count++;
    return 0;
}

message_t* dequeue_message(ring_buffer_t* buf){
    if(get_queue_used(buf) <= 0){
        return NULL;
    }
    message_t* msg = (message_t*)buf->buffer[buf->head];
    buf->buffer[buf->head] = NULL;
    buf->head = (buf->head + 1) % QUEUE_SIZE;
    buf->removed_count++;
    return msg;
}
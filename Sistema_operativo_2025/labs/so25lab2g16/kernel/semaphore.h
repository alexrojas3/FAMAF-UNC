#ifndef SEMAPHORE_H
#define SEMAPHORE_H

#include <stdbool.h>

#define MAX_SEM 260
#define CLOSED_SEM_VAL -1

typedef struct semaphore_t {
    bool is_use;
    int value;
    struct spinlock lock;
} semaphore;


#endif
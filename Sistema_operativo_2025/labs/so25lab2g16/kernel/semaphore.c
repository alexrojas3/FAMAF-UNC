#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"
#include "semaphore.h"
#include "stdbool.h"

// II) Array semaphore
semaphore sem_array[MAX_SEM];

// III) Syscall
int 
sem_init(void)
{
    for (int i = 0; i < MAX_SEM; i++) {
        initlock(&sem_array[i].lock, "sem_lock");
        sem_array[i].is_use = false;
        sem_array[i].value = CLOSED_SEM_VAL; // todos empiezan cerrados 
    }
    return 1;
}

int 
sem_open(int sem, int value)
{
    acquire(&sem_array[sem].lock);
    
    // si esta en uso o valor negativo devuelve error
    if (sem_array[sem].is_use) {
        release(&sem_array[sem].lock);
        return 0;
    }
    // aunque verifiquemos en el progrma en espacio usuario porq seria para uso gral de usuarios
    // no solamente ese programa 
    if (value < 0){
        release(&sem_array[sem].lock);
        return 0;
    }
    // si no esta en uso
    sem_array[sem].value = value;
    sem_array[sem].is_use = true;
    release(&sem_array[sem].lock);
    return 1;
}

int 
sem_close(int sem)
{
    acquire(&sem_array[sem].lock);

    if (!sem_array[sem].is_use) {
        release(&sem_array[sem].lock);
        return 0;
    }
    sem_array[sem].is_use = false;
    sem_array[sem].value = CLOSED_SEM_VAL; // valor base para semafporos cerrados
    wakeup(&sem_array[sem]);

    release(&sem_array[sem].lock);
    return 1;
}

int 
sem_up(int sem)
{
    acquire(&sem_array[sem].lock);

    if (!sem_array[sem].is_use) {
        release(&sem_array[sem].lock);
        return 0;
    }
    
    if (sem_array[sem].value == 0) { // si es 0 desbloqueamos los procesos
        wakeup(&sem_array[sem]);
    }
    sem_array[sem].value++;

    release(&sem_array[sem].lock);
    return 1;
}

int 
sem_down(int sem)
{
    acquire(&sem_array[sem].lock);

    if (!sem_array[sem].is_use) { // verificamos que este en uso
        release(&sem_array[sem].lock);
        return 0;
    }

    while (sem_array[sem].value == 0) {
        sleep(&sem_array[sem], &sem_array[sem].lock);
        if (sem_array[sem].value == CLOSED_SEM_VAL) { // si es -1 entonces lo cerraron mientras dormia
            release(&sem_array[sem].lock);
            return 0;
        }
    }
    sem_array[sem].value--;

    release(&sem_array[sem].lock);
    return 1;
}
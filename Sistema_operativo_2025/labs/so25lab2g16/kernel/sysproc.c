#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"
#include "semaphore.h"

uint64
sys_exit(void)
{
  int n;
  argint(0, &n);
  exit(n);
  return 0;  // not reached
}

uint64
sys_getpid(void)
{
  return myproc()->pid;
}

uint64
sys_fork(void)
{
  return fork();
}

uint64
sys_wait(void)
{
  uint64 p;
  argaddr(0, &p);
  return wait(p);
}

uint64
sys_sbrk(void)
{
  uint64 addr;
  int n;

  argint(0, &n);
  addr = myproc()->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

uint64
sys_sleep(void)
{
  int n;
  uint ticks0;

  argint(0, &n);
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(killed(myproc())){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

uint64
sys_kill(void)
{
  int pid;

  argint(0, &pid);
  return kill(pid);
}

// return how many clock tick interrupts have occurred
// since start.
uint64
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

/* 1. sys_sem_open()*/
uint64
sys_sem_open(void)
{
  int sem;
  int value;
  argint(0, &sem);
  argint(1, &value);

  if (sem < 0 || sem >= MAX_SEM) return -1;
  if (value < 0) return -1;

  // luego de validar los parametros
  return (uint64)sem_open(sem, value);
}

/* 2. sys_sem_close()*/
uint64
sys_sem_close(void)
{
  int sem;
  argint(0, &sem);

  if (sem < 0 || sem >= MAX_SEM) return -1;

  return (uint64)sem_close(sem);
}

/* 3. sys_sem_up()*/
uint64
sys_sem_up (void)
{
  int sem;
  argint(0, &sem);

  if (sem < 0 || sem >= MAX_SEM) return -1;

  return (uint64)sem_up(sem);
}

/* 4. sys_sem_down() */
uint64
sys_sem_down(void)
{
  int sem;
  argint(0, &sem);

  if (sem < 0 || sem >= MAX_SEM) return -1;

  return (uint64)sem_down(sem);
}

uint64
sys_sem_init(void)
{
  return (uint64)sem_init(); // no tenemos parametros para validar
}
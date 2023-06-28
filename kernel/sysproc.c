#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"

uint64
sys_exit(void)
{
  int n;
  if(argint(0, &n) < 0)
    return -1;
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
  if(argaddr(0, &p) < 0)
    return -1;
  return wait(p);
}

uint64
sys_sbrk(void)
{
  int addr;
  int n;

  if(argint(0, &n) < 0)
    return -1;
  addr = myproc()->sz;
  // if(growproc(n) < 0)
  //   return -1;

  uint64 sz = myproc()->sz;
  if(n>0)
  {
      sz += n;
  }else{
    if(sz+n<0){
      return -1;
    }else{
      //处理sbrk()参数为负的情况
      sz = uvmdealloc(myproc()->pagetable,sz,sz+n);
    }
  }
  myproc()->sz = sz;
  return addr;
}

uint64
sys_sleep(void)
{
  int n;
  uint ticks0;

  if(argint(0, &n) < 0)
    return -1;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(myproc()->killed){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  backtrace();
  return 0;
}

uint64
sys_kill(void)
{
  int pid;

  if(argint(0, &pid) < 0)
    return -1;
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
uint64
sys_sigalarm(void)
{
  int ticks;
  uint64 handler;

  if(argint(0,&ticks)<0) return -1;

  argaddr(1,&handler);
  struct proc *p=myproc();
  p->ticks = ticks;
  p->handler = handler;
  p->ticks_cnt = 0;

  return 0;
}

void
restrore(void)
{
  struct proc* p = myproc();
  p->trapframe->ra = p->ticks_ra;
  p->trapframe->sp = p->ticks_sp;
  p->trapframe->gp = p->ticks_gp;
  p->trapframe->tp = p->ticks_tp;
  p->trapframe->t0 = p->ticks_t0;
  p->trapframe->t1 = p->ticks_t1;
  p->trapframe->t2 = p->ticks_t2;
  p->trapframe->s0 = p->ticks_s0;
  p->trapframe->s1 = p->ticks_s1;
  p->trapframe->a0 = p->ticks_a0;
  p->trapframe->a1 = p->ticks_a1;
  p->trapframe->a2 = p->ticks_a2;
  p->trapframe->a3 = p->ticks_a3;
  p->trapframe->a4 = p->ticks_a4;
  p->trapframe->a5 = p->ticks_a5;
  p->trapframe->a6 = p->ticks_a6;
  p->trapframe->a7 = p->ticks_a7;
  p->trapframe->s2 = p->ticks_s2;
  p->trapframe->s3 = p->ticks_s3;
  p->trapframe->s4 = p->ticks_s4;
  p->trapframe->s5 = p->ticks_s5;
  p->trapframe->s6 = p->ticks_s6;
  p->trapframe->s7 = p->ticks_s7;
  p->trapframe->s8 = p->ticks_s8;
  p->trapframe->s9 = p->ticks_s9;
  p->trapframe->s10 = p->ticks_s10;
  p->trapframe->s11 = p->ticks_s11;
  p->trapframe->t3 = p->ticks_t3;
  p->trapframe->t4 = p->ticks_t4;
  p->trapframe->t5 = p->ticks_t5;
  p->trapframe->t6 = p->ticks_t6;


}

uint64
sys_sigreturn(void)
{
  struct proc *p=myproc();
  p->trapframe->epc = p->ticks_epc;
  restrore();
  p->handler_executing = 0;
  return 0;
}

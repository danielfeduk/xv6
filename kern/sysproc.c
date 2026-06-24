#include "types.h"
#include "defs.h"
// this is bad
#include "param.h"
#include "mmu.h"
#include "x86.h"
#include "proc.h"

int
sys_nosys(void)
{
	return -1;
}

int
sys_uptime(void)
{
	u64 xticks;
	acquire(&tickslock);
	xticks = ticks;
	release(&tickslock);
	return xticks;
}

int
sys_sleep(int n)
{
	u64 ticks0;

	acquire(&tickslock);
	ticks0 = ticks;
	while (ticks - ticks0 < n) {
		if(myproc()->killed) {
			release(&tickslock);
			return -1;
		}
		sleep(&ticks, &tickslock);
	}
	release(&tickslock);
	return 0;
}

int
sys_debugprnt(const char *str)
{
	cprintf("cpu %d proc %d says: %s\n", cpuid(), myproc()->pid, str);
	return 0;
}

int
sys_debugprnt2(u64 n)
{
	cprintf("cpu %d proc %d says: %d\n", cpuid(), myproc()->pid, n);
	return 0;
}

int
sys_getpid(void)
{
	return myproc()->pid;
}

int
sys_exit(int status)
{
	exit(status);
	return 0; // unreachable
}

int
sys_wait(int *status)
{
	int *s = userbuf(status, sizeof(int));
	if(!s) return -1;
	return wait(s);
}

int
sys_kill(int pid)
{
	if (pid < 0)
		return -1;

	return kill(pid);
}

int
sys_fork(void)
{
	cprintf("fork!\n");
	return fork();
}

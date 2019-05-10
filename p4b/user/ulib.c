#include "types.h"
#include "stat.h"
#include "fcntl.h"
#include "user.h"
#include "x86.h"
//#include "mmu.h"
#include "param.h"

#define PGSIZE 4096

typedef struct __stack_info {
	int pid;
	void *mem;
} stack_info;

static stack_info stacks[NPROC];

char*
strcpy(char *s, char *t)
{
  char *os;

  os = s;
  while((*s++ = *t++) != 0)
    ;
  return os;
}

int
strcmp(const char *p, const char *q)
{
  while(*p && *p == *q)
    p++, q++;
  return (uchar)*p - (uchar)*q;
}

uint
strlen(char *s)
{
  int n;

  for(n = 0; s[n]; n++)
    ;
  return n;
}

void*
memset(void *dst, int c, uint n)
{
  stosb(dst, c, n);
  return dst;
}

char*
strchr(const char *s, char c)
{
  for(; *s; s++)
    if(*s == c)
      return (char*)s;
  return 0;
}

char*
gets(char *buf, int max)
{
  int i, cc;
  char c;

  for(i=0; i+1 < max; ){
    cc = read(0, &c, 1);
    if(cc < 1)
      break;
    buf[i++] = c;
    if(c == '\n' || c == '\r')
      break;
  }
  buf[i] = '\0';
  return buf;
}

int
stat(char *n, struct stat *st)
{
  int fd;
  int r;

  fd = open(n, O_RDONLY);
  if(fd < 0)
    return -1;
  r = fstat(fd, st);
  close(fd);
  return r;
}

int
atoi(const char *s)
{
  int n;

  n = 0;
  while('0' <= *s && *s <= '9')
    n = n*10 + *s++ - '0';
  return n;
}

void*
memmove(void *vdst, void *vsrc, int n)
{
  char *dst, *src;
  
  dst = vdst;
  src = vsrc;
  while(n-- > 0)
    *dst++ = *src++;
  return vdst;
}

int
thread_create(void (*start_routine)(void *, void *), void *arg1, void *arg2)
{
	void *stack;
	int pid;
	//allocate page-aligned PGSIZE
	void *mem = malloc(PGSIZE * 2);
	if (mem == NULL)
		return -1;
	uint mask = PGSIZE - 1;
	stack = (void *) (((uint) mem + mask) & ~mask);

	//call clone
	pid = clone(start_routine, arg1, arg2, stack);
	if (pid < 0) {
		free(mem);
		return pid;
	}
	//find free spot in stacks[] and store pid + pointer
	for (int i = 0; i < NPROC; ++i) {
		if (stacks[i].pid == 0) {
			stacks[i].pid = pid;
			stacks[i].mem = mem;
			break;
		}
	}

	//return PID to parent, 0 to child; or return -1 on error
	return pid;
}

int
thread_join()
{
	void **stack = NULL;
	int pid = join(stack);

	if (pid >= 0) {
		//find mem pointer in stacks[]
		for (int i = 0; i < NPROC; ++i) {
			if (stacks[i].pid == pid) {
				free(stacks[i].mem);
				stacks[i].pid = 0;
				break;
			}
		}
	}

	return pid;
}

static inline int
fetch_and_add(int *var, int val)
{
	__asm__ volatile("lock; xaddl %0, %1"
		: "+r" (val), "+m" (*var) //input+output
		: //no input-only
		: "memory"
	);
	return val;
}

void
lock_init(lock_t *lock)
{
	lock->ticket = 0;
	lock->turn = 0;
}

void
lock_acquire(lock_t *lock)
{
	int myturn = fetch_and_add(&lock->ticket, 1);
	while (lock->turn != myturn)
		; //spin wait
}

void
lock_release(lock_t *lock)
{
	lock->turn = lock->turn + 1;
}

#ifndef _PROCESS_H_
#define _PROCESS_H_

#include <config.h>
#include <fs/file.h>
#include <kernel/signal.h>
#include <kernel/types.h>
#include <mm/paging.h>

/** Registers structure. **/

struct registers
{
	ui32_t gs, fs, es, ds,
	       edi, esi,
	       ebp,
	       ebx, edx, ecx, eax,
	       eip, cs,
	       eflags,
	       esp, ss;
} __attribute__((packed));

/** Process structure. **/

struct process
{
	/** The two first members must be kept intact. Their offset in the
	    structure is hard-coded. **/

	struct registers regs;
	ui32_t *pd;
	struct registers regs2;

	/** (Do not add anything above this limit.) **/

	bool_t used;

	pid_t pid, pgid;

	struct pheap_page kstack_page;
	ui32_t esp0;
	void *b_heap;
	void *e_heap;

	struct process *parent;
	struct process *first_son;
	struct process *prev_sibling, *next_sibling;

	struct fildes *pfildes_tab[NR_FILDES_PER_PROC];
	ui32_t fildes_flags[NR_FILDES_PER_PROC];
	uchar_t cwd[PATH_MAX_LEN + 1];

	sigset_t sigset;
	sigset_t sigmask;
	struct sigaction sigact[31];
	bool_t interruptible; // the process is in an interruptible syscall

	enum {
		PROC_ZOMBIE,
		PROC_NOT_RUNNABLE,
		PROC_READY, // ready or running
		PROC_SUSPENDED // suspended process
	} state;
	int status; // Exit status
	bool_t reported;
	pid_t dead_son_pid;
	int dead_son_status;

	ui32_t *perrno;

	ui32_t sysc_num;
};

/** Global variables. **/

#ifdef _PROCESS_C_
count_t nproc = 0;
struct process proc_tab[NR_PROC] = {
	{
		.used = 0
	}
};
pid_t current_pid = 0;
struct process *current = &proc_tab[0];
#else
extern count_t nproc;
extern struct process proc_tab[];
extern pid_t current_pid;
extern struct process *current;
#endif

/** Functions **/

pid_t process_create(struct process *parent, void *code, size_t code_size);

#endif

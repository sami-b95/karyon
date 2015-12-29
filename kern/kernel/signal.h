#ifndef _SIGNAL_H_
#define _SIGNAL_H_

#include <kernel/types.h>

/** sigset_t type **/

typedef ui32_t sigset_t;

/** sigaction structure **/

struct sigaction
{
	void (*sa_handler)(ui32_t);
	sigset_t sa_mask;
	ui32_t sa_flags;
	void (*sa_restorer)(void);
};

/** Signals **/

#define SIGHUP		 1
#define SIGINT		 2
#define SIGQUIT		 3
#define SIGABRT		 6
#define SIGBUS		 7
#define SIGKILL		 9
#define SIGUSR1		10
#define SIGSEGV		11
#define SIGUSR2		12
#define SIGPIPE		13
#define SIGALRM		14
#define SIGTERM		15
#define SIGCHLD		17
#define SIGCONT		18
#define SIGSTOP		19
#define SIGTSTP		20
#define SIGTTIN		21
#define SIGTTOU		22

/** sa_handler field in sigaction structure **/

#define SIG_DFL         (void*)0
#define SIG_IGN         (void*)1

/** sa_flags field in sigaction structure **/

#define SA_SIGINFO      1
#define SA_RESETHAND    2
#define SA_NODEFER	4

/** Value for parameter how in function sigprocmask **/

#define SIG_BLOCK	1
#define SIG_UNBLOCK	2
#define SIG_SETMASK	3

/** Functions **/

ui32_t signal_dequeue(sigset_t sigset, sigset_t sigmask);
/****************************************************************/
void signal_handle(ui32_t sig);
/****************************************************************/
count_t signal_send_pgrp(ui32_t sig, pid_t pgid);

#endif

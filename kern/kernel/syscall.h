#ifndef _SYSCALL_H_
#define _SYSCALL_H_

#include <fs/file.h>
#include <fs/tty.h>
#include <kernel/errno.h>
#include <kernel/signal.h>
#include <kernel/stdarg.h>
#include <kernel/types.h>

/** Syscall IDs **/

#define SYSCALL_PRINT		0x00
#define SYSCALL_PRINT_NUMBER	0x01

#define SYSCALL_FORK		0x20
#define SYSCALL_SIGRET		0x21
#define SYSCALL_KILL		0x22
#define SYSCALL_SIGACTION	0x23
#define SYSCALL_EXIT		0x24
#define SYSCALL_EXECVE		0x26
#define SYSCALL_GETPID		0x27
#define SYSCALL_SETPGID		0x28
#define SYSCALL_SIGPROCMASK	0x2a
#define SYSCALL_KILLPG		0x2b
#define SYSCALL_GETPPID		0x2c
#define SYSCALL_GETPGID		0x2e
#define SYSCALL_SIGSUSPEND	0x2f
#define SYSCALL_SBRK		0x30
#define SYSCALL_WAIT4		0x31

#define SYSCALL_OPEN		0x40
#define SYSCALL_CLOSE		0x41
#define SYSCALL_READ		0x42
#define SYSCALL_WRITE		0x43
#define SYSCALL_LSEEK		0x44
#define SYSCALL_LINK		0x45
#define SYSCALL_UNLINK		0x46
#define SYSCALL_FSTAT		0x47
#define SYSCALL_ISATTY		0x48
#define SYSCALL_FCNTL		0x49
#define SYSCALL_CHDIR		0x4a
#define SYSCALL_GETCWD		0x4b
#define SYSCALL_DUP2		0x4c
#define SYSCALL_MKDIR		0x4d
#define SYSCALL_RMDIR		0x4e
#define SYSCALL_SYNC		0x4f
#define SYSCALL_SELECT		0x50
#define SYSCALL_GETDTABLESIZE	0x51
#define SYSCALL_PIPE2		0x52
#define SYSCALL_FTRUNCATE	0x53
#define SYSCALL_FSTATFS		0x54

#define SYSCALL_TCGETATTR	0x60
#define SYSCALL_TCSETATTR	0x61
#define SYSCALL_TCFLUSH		0x62
#define SYSCALL_TCGETPGRP	0x63
#define SYSCALL_TCSETPGRP	0x64

#define SYSCALL_SOCKET		0xa0
#define SYSCALL_CONNECT		0xa1
#define SYSCALL_BIND		0xa2
#define SYSCALL_LISTEN		0xa3
#define SYSCALL_ACCEPT		0xa4
#define SYSCALL_RECV		0xa5
#define SYSCALL_SEND		0xa6
#define SYSCALL_GETSOCKOPT	0xa7
#define SYSCALL_SETSOCKOPT	0xa8
#define SYSCALL_GETSOCKNAME	0xa9

#define SYSCALL_NETCONF		0xc0

/** Syscall macros **/

#define syscall(sysc_num, param, ret) \
	asm volatile("mov %1, %%eax \n\
	              lea (dummy_errno), %%ebx \n\
	              mov %2, %%esi \n\
	              int $0x30 \n\
	              mov %%eax, %0" : "=m"(ret) \
	                             : "i"(sysc_num), "m"(param) \
	                             : "eax", "ebx", "esi", "memory")

#define syscall_no_param(sysc_num, ret) { \
	void *__param = 0; \
	syscall(sysc_num, __param, ret); \
}

#define syscall_no_ret(sysc_num, param) { \
	ui32_t __ret = 0; \
	syscall(sysc_num, param, __ret); \
}

#define syscall_no_param_no_ret(sysc_num) { \
	void *__param = 0; \
	ui32_t __ret = 0; \
	syscall(sysc_num, __param, __ret); \
}

/** Syscall ISR **/

void _isr_syscall();

/** Syscalls **/

pid_t sys_fork();
/****************************************************************/
int sys_kill(pid_t pid, ui32_t sig);
/****************************************************************/
void sys_sigret();
/****************************************************************/
int sys_sigaction(ui32_t sig, struct sigaction *act, struct sigaction *oldact);
/****************************************************************/
void sys_exit(int status);
/****************************************************************/
si32_t sys_open(uchar_t *path, ui32_t flags);
/****************************************************************/
int sys_close(si32_t fildes);
/****************************************************************/
int sys_execve(uchar_t *bin, uchar_t *argv[], uchar_t *envp[]);
/****************************************************************/
ssize_t sys_read(si32_t fildes, void *buf, size_t size);
/****************************************************************/
ssize_t sys_write(si32_t fildes, void *buf, size_t size);
/****************************************************************/
int sys_link(uchar_t *old_path, uchar_t *new_path);
/****************************************************************/
int sys_unlink(uchar_t *path);
/****************************************************************/
int sys_fstat(si32_t fildes, struct stat *st);
/****************************************************************/
off_t sys_lseek(si32_t fildes, off_t off, ui32_t whence);
/****************************************************************/
int sys_isatty(si32_t fildes);
/****************************************************************/
si32_t sys_fcntl(si32_t fildes, ui32_t cmd, va_list ap);
/****************************************************************/
int sys_tcgetattr(si32_t fildes, struct termios *termios_p);
/****************************************************************/
int sys_tcsetattr(si32_t fildes, ui32_t opt_act, struct termios *termios_p);
/****************************************************************/
int sys_dup2(si32_t old_fildes, si32_t new_fildes);
/****************************************************************/
int sys_sigprocmask(ui32_t how, sigset_t *set, sigset_t *old_set);
/****************************************************************/
int sys_killpg(pid_t pgid, ui32_t sig);
/****************************************************************/
int sys_chdir(uchar_t *path);
/****************************************************************/
uchar_t *sys_getcwd(uchar_t *buf, size_t size);
/****************************************************************/
pid_t sys_wait4(pid_t pid, int *status, int options, struct rusage *rusage);
/****************************************************************/
int sys_tcflush(si32_t fildes, ui32_t queue_selector);
/****************************************************************/
int sys_mkdir(uchar_t *path, mode_t mode);
/****************************************************************/
int sys_rmdir(uchar_t *path);
/****************************************************************/
int sys_sigsuspend(sigset_t *mask);
/****************************************************************/
pid_t sys_tcgetpgrp(si32_t fildes);
/****************************************************************/
pid_t sys_tcsetpgrp(si32_t fildes, pid_t pg);
/****************************************************************/
si32_t sys_select(ui32_t nfds,
                  fd_set *readfds,
                  fd_set *writefds,
                  fd_set *exceptfds,
                  struct timeval *timeout);
/****************************************************************/
int sys_pipe2(si32_t pipefd[2], ui32_t flags);
/****************************************************************/
int sys_ftruncate(si32_t fildes, off_t length);
/****************************************************************/
int sys_fstatfs(si32_t fildes, struct statfs *st);
/****************************************************************/
int sys_socket(ui32_t domain, ui32_t type, ui32_t proto);
/****************************************************************/
int sys_connect(si32_t fildes, struct sockaddr *addr, socklen_t addr_len);
/****************************************************************/
int sys_bind(si32_t fildes, struct sockaddr *addr, socklen_t addr_len);
/****************************************************************/
int sys_listen(si32_t fildes, ui32_t backlog);
/****************************************************************/
si32_t sys_accept(si32_t fildes, struct sockaddr *addr, socklen_t *addr_len);
/****************************************************************/
ssize_t sys_recv(si32_t fildes, void *buf, size_t len, ui32_t flags);
/****************************************************************/
ssize_t sys_send(si32_t fildes, void *buf, size_t len, ui32_t flags);
/****************************************************************/
int sys_getsockopt(si32_t fildes,
                   ui32_t level,
                   ui32_t opt_name,
                   void *opt_val,
                   socklen_t *opt_len);
/****************************************************************/
int sys_setsockopt(si32_t fildes,
                   ui32_t level,
                   ui32_t opt_name,
                   void *opt_val,
                   socklen_t opt_len);
/****************************************************************/
int sys_getsockname(si32_t fildes, struct sockaddr *addr, socklen_t *len);

#endif

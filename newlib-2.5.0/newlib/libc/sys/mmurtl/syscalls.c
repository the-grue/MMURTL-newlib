/* note these headers are all provided by newlib - you don't need to provide them */
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/fcntl.h>
#include <sys/times.h>
#include <sys/errno.h>
#include <sys/time.h>
#include <stdio.h>
#include <mmlib.h>
 
/*  void _exit(int ExitCode); */

void _exit(int ExitCode)
{
	ExitJob(ExitCode);
}

/* int close(int file); */

int close(int file)
{
	/*  MMURTL CloseFile will not allow close of file handles */
	/*  0-2 and will just return 0 if you select one of them. */
	return CloseFile(file);
}

/*  int getpid(); */

int getpid(void)
{
	int pdJobNumRet;

	GetJobNum(&pdJobNumRet);

	return pdJobNumRet;
}

/*  int isatty(int file); */

int isatty(int file)
{
	/* MMURTL only uses 1 for stdin and 2 for stdout and stderr */
	/* per the stdio.h file used with CM32 but using 0-2 for newlib */
	if((file >= 0) && (file <= 2))
		return 1;
	else
		return 0;
}

/*  int kill(int pid, int sig); */

int kill(int pid, int sig)
{
	/* No real signals in MMURTL, so we just terminate either the */
	/* requesting process passing sig as the exit code or kill another */
	/* job with KillJob and ignore sig.  This may return an error. */
	if(getpid() == pid)
		_exit(sig);
	else
		return KillJob(pid);
}

char **environ; /* pointer to array of char * strings that define the current environment variables */
int execve(char *name, char **argv, char **env);
int fork();
int fstat(int file, struct stat *st);
int link(char *old, char *new);
int lseek(int file, int ptr, int dir);
int open(const char *name, int flags, ...);
int read(int file, char *ptr, int len);
caddr_t sbrk(int incr);
int stat(const char *file, struct stat *st);
clock_t times(struct tms *buf);
int unlink(char *name);
int wait(int *status);
int write(int file, char *ptr, int len);
/*int gettimeofday(struct timeval *p, struct timezone *z);*/
int gettimeofday(struct timeval *p, void *z);

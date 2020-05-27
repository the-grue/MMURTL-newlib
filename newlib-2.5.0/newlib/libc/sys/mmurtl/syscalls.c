/* note these headers are all provided by newlib - you don't need to provide them */
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/fcntl.h>
#include <sys/times.h>
#include <sys/errno.h>
#include <sys/time.h>
#include <stdio.h>
#include <string.h>
#include <mmlib.h>

#define ERCOK		0
#define ERCNOTAFILE	202
#define ERCDUPNAME	226

static int *pagebreak = 0xFFFFFFFF;

int lseek(int file, int ptr, int dir);
 
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

/* int open(const char *name, int flags, ...); */
int open(const char *name, int flags, ...)
{
	int pdHandleRet;
	int erc;
	int fnamelen = strlen(name);

	/* We'll only try to check for flags that are supported by */
	/* MMURTL and group as required.  For example, we'll ignore  */
	/* the ... for mode since FAT doesn't support the Unix type */
	/* file permissions rwxrwxrwx */

	/* First, see if we are trying to create a file */

	if(flags & O_CREAT)
	{
		erc = CreateFile(name, fnamelen, 0);	

		if(erc == ERCNOTAFILE)
		{
			errno = EINVAL;
			return -1;
		}
		if(erc == ERCDUPNAME)
		{
			if(flags & O_EXCL)
			{
				errno = EINVAL;
				return -1;
			}
			if(flags & O_TRUNC)
			{
				erc = OpenFile(name, fnamelen, 1, 1, &pdHandleRet);
				if(erc != ERCOK)
				{
					errno = EINVAL;
					return -1;
				}
				erc = DeleteFile(pdHandleRet);
				if(erc != ERCOK)
				{
					errno = EINVAL;
					return -1;
				}
				erc = CreateFile(name, fnamelen, 0);
				if(erc != ERCOK)
				{
					errno = EINVAL;
					return -1;
				}

			}
		}
	}

	/* Now open the file if we got to here */

	erc = OpenFile(name, fnamelen, ((flags & 2) >> 1), 1, &pdHandleRet);
	if(erc != ERCOK)
	{
		errno = EINVAL;
		return -1;
	}

	if(flags & O_APPEND)
		if(lseek(pdHandleRet, 0, SEEK_END) == -1)
		{
			close(pdHandleRet);
			errno = EINVAL;
			return -1;
		}

	return pdHandleRet;
}

/* int lseek(int file, int ptr, int dir); */

int lseek(int file, int ptr, int dir)
{
	int lfa;
	int size;
	int erc;

	GetFileSize(file, &size);
				
	switch(dir)
	{
		case SEEK_SET:
		      		if(ptr < 0 || ptr > size)
				{
					errno = EINVAL;
					return -1;
				}
		      		lfa = ptr;
				break;

		case SEEK_CUR:
		      		GetFileLFA(file, &lfa);
				lfa += ptr;
				if(lfa < 0 || lfa > size)
				{
					errno = EINVAL;
					return -1;
				}
				break;

		case SEEK_END:
		      		lfa = ptr + size;
				if(lfa < 0)
				{
					errno = EINVAL;
					return -1;
				}
		      		if(ptr > 0)
		      			lfa = -1;
				break;

		default:
				errno = EINVAL;
				return -1;
	}

	erc = SetFileLFA(file, lfa);

	if(erc)
	{
		errno = EINVAL;
		return -1;
	}
	else
		return lfa;
}

/* int read(int file, char *ptr, int len); */

int read(int file, char *ptr, int len)
{
	int pdBytesRet;
	int erc;
       
	/* Since MMURTL uses 1 for keyboard, remap reads */
	/* for file handle 0 (stdin) to file handle 1 */

	if(file == 0)
		file = 1;

	erc = ReadBytes(file, ptr, len, &pdBytesRet);

	if(erc)
	{
		errno = EINVAL;
		return -1;
	}
	else
		return pdBytesRet;
}

/* int write(int file, char *ptr, int len); */

int write(int file, char *ptr, int len)
{
	int *pdBytesRet;
	int erc; 

	/* Since MMURTL uses 1 for keyboard, remap writes */
	/* for file handle 1 to file handle 2, which also */
	/* covers stderr */
	if(file == 1)
		file = 2;

	erc = WriteBytes(file, ptr, len, &pdBytesRet);

	if(erc)
	{
		errno = EINVAL;
		return -1;
	}
	else
		return pdBytesRet;
}

/* int link(char *old, char *new); */

int link(char *old, char *new)
{
	/* MMURTL doesn't support file linking.  We could make a copy */
	/* of the file, but for now we'll just return an error.  */
	errno = EMLINK;
	return -1;
}

/* int unlink(char *name); */

int unlink(char *name)
{
	int delfile;
	int erc;

	/* MMURTL DeleteFile system call takes a file handle to an */
	/* open file to delete a file, so we have to open the file first. */
	if((delfile = open(name, O_RDWR)) < 0)
	{
		errno = ENOENT;
		return -1;
	}

	erc = DeleteFile(delfile);

	if(erc)
	{
		errno = EINVAL;
		return -1;
	}
	else
		return erc;
}

/* caddr_t sbrk(int incr); */

caddr_t sbrk(int incr)
{
	int pages;
	char *ppMemRet;
	int erc;

	if(incr == 0)
	{
		if(*pagebreak == 0xFFFFFFFF)
		{
			erc = AllocPage(1, &ppMemRet);
			if(erc)
			{
				errno = ENOMEM;
				return -1;
			}	
			pagebreak = ppMemRet + 0x1000 - 1;
			return pagebreak;
		}
		else
			return pagebreak;
	}

	pages = incr / 4096;
	if(incr % 4096)
		pages++;

	erc = AllocPage(pages, &ppMemRet);

	if(erc)
	{
		errno = ENOMEM;
		return -1;
	}	
	else
	{
		pagebreak = ppMemRet + (pages * 0x1000) - 1;
		return ppMemRet;
	}
}


char **environ; /* pointer to array of char * strings that define the current environment variables */
int execve(char *name, char **argv, char **env);
int fork();
int fstat(int file, struct stat *st);
int stat(const char *file, struct stat *st);
clock_t times(struct tms *buf);
int wait(int *status);
/*int gettimeofday(struct timeval *p, struct timezone *z);*/
int gettimeofday(struct timeval *p, void *z);

#include <stdint.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/ptrace.h>
#include <stdio.h>
#include <errno.h>
#include <sys/wait.h>
#include <signal.h>
#include <pthread.h>
#include <sys/user.h>
#include <sys/reg.h>
#include <sys/time.h>

#include "tracee.h"

//#define DEBUG_PTRACE

static inline double now(void)
{
	struct timeval tv;

	if(gettimeofday(&tv, NULL) < 0)
	{
		return 0.0;
	}

	double sec  = tv.tv_sec;
	double usec = tv.tv_usec * 1e-6;

	return sec + usec;
}

/*****************************
* INTERNAL TRACEE INTERFACE *
*****************************/

typedef enum
{
	TRACEE_PTRACE,
	TRACEE_LINUX_PERF
}tracee_kind_t;

struct tracee_s
{
	tracee_kind_t     kind;
	pid_t             pid;

	/* Ptrace specific */
	int               attached;
	int               running;
	int               tracked;
	tracee_callback_t cb;
};


tracee_error_t tracee_s_continue_ptrace(struct tracee_s *ts, int sig)
{
	long long lsig = sig;
	void *    asig = (void *)lsig;
	long      ret  = ptrace(PTRACE_CONT, ts->pid, NULL, asig);


#ifdef DEBUG_PTRACE
	fprintf(stderr, "PTRACE continue on %d with sig %d\n", ts->pid, sig);
#endif

	if(ret < 0)
	{
		perror("ptrace (cont)");
		return TRACEE_ERR_OTHER;
	}

	ts->running = 1;


	return TRACEE_SUCCESS;
}

tracee_error_t tracee_s_wait_for_sig_ptrace(struct tracee_s *ts, int signal_id, int *with__bit_7)
{
	while(1)
	{
#ifdef DEBUG_PTRACE
		fprintf(stderr, "wait for sig waiting on %d for sig %d\n", ts->pid, signal_id);
#endif


		int   wstatus;
		pid_t ret = waitpid(ts->pid, &wstatus, 0);

		if(ret < 0)
		{
			perror("waitpid");
			ts->attached = 0;
			ts->running  = 0;
			return TRACEE_ERR_PERM;
		}

		ts->running = 0;

		if(WIFEXITED(wstatus) )
		{
	    #ifdef DEBUG_PTRACE
			fprintf(stderr, "%d has exited\n", ts->pid);
	    #endif
			return TRACEE_ERR_EXITED;
		}

		if(WIFSTOPPED(wstatus) )
		{
			int sig = WSTOPSIG(wstatus);

	    #ifdef DEBUG_PTRACE
			fprintf(stderr, "%d has stopped with sig %d\n", ts->pid, sig);
	    #endif

			if(with__bit_7)
			{
				*with__bit_7 = sig & 0x80;
			}

			if( ( (sig & ~0x80) == signal_id) || (!signal_id) )
			{
		#ifdef DEBUG_PTRACE
				fprintf(stderr, "Done waiting on %d for sig %d\n", ts->pid, sig);
		#endif
				return TRACEE_SUCCESS;
			}
			else
			{
				fprintf(stderr, "wait for sig forwarding to %d sig %d\n", ts->pid, sig);

				/* Forward the signal back to the tracee */
				tracee_error_t ce = tracee_s_continue_ptrace(ts, sig);

				if(ce != TRACEE_SUCCESS)
				{
					return ce;
				}
			}
		}
	}

	return TRACEE_ERR_UNREACHABLE;
}

tracee_error_t tracee_s_interrupt_ptrace(struct tracee_s *ts)
{
	long ret = ptrace(PTRACE_INTERRUPT, ts->pid, NULL, NULL);

	if(ret < 0)
	{
		perror("ptrace (intr)");
		return TRACEE_ERR_OTHER;
	}

	/* Wait for stop */
	tracee_error_t rc = tracee_s_wait_for_sig_ptrace(ts, SIGSTOP, NULL);

	if(rc != TRACEE_SUCCESS)
	{
		return rc;
	}

	ts->running = 0;

#ifdef DEBUG_PTRACE
	fprintf(stderr, "PTRACE interrupt on %d\n", ts->pid);
#endif

	return TRACEE_SUCCESS;
}

tracee_error_t tracee_s_tracksyscalls_ptrace(struct tracee_s *ts)
{
	long ret = ptrace(PTRACE_SYSCALL, ts->pid, NULL, NULL);

	if(ret < 0)
	{
		perror("ptrace (syscalls)");
		return TRACEE_ERR_OTHER;
	}

	ts->running = 1;

#ifdef DEBUG_PTRACE
	fprintf(stderr, "PTRACE track syscalls on %d\n", ts->pid);
#endif

	return TRACEE_SUCCESS;
}

tracee_error_t tracee_s_attach_ptrace(struct tracee_s *ts)
{
	long ret = ptrace(PTRACE_ATTACH, ts->pid, NULL, NULL);

	if(ret < 0)
	{
		perror("ptrace (attach)");
		ts->attached = 0;
		return TRACEE_ERR_PERM;
	}

	ts->attached = 1;

	/* Wait for stop */
	tracee_error_t rc = tracee_s_wait_for_sig_ptrace(ts, SIGSTOP, NULL);

	if(rc != TRACEE_SUCCESS)
	{
		return rc;
	}

	/* This sets bit 7 in syscall traps to help distinguishing them */
	if(ptrace(PTRACE_SETOPTIONS, ts->pid, 0, PTRACE_O_TRACESYSGOOD) < 0)
	{
		perror("ptrace (setoption)");
		return TRACEE_ERR_OTHER;
	}

	ts->tracked = 0;

	/* Now pursue execution */
	return TRACEE_SUCCESS;
}

tracee_error_t tracee_s_detach_ptrace(struct tracee_s *ts)
{
	long ret = ptrace(PTRACE_DETACH, ts->pid, NULL, NULL);

	if(ret < 0)
	{
		perror("ptrace (detach)");
		ts->attached = -1;
		return TRACEE_ERR_PERM;
	}
	else
	{
		ts->attached = 0;
		return TRACEE_SUCCESS;
	}

	return TRACEE_ERR_UNREACHABLE;
}

static tracee_error_t __ptrace_syscall_tracker(struct tracee_s *ts)
{
	if(ts->running)
	{
		/* If running iterrupt first */
		tracee_s_interrupt_ptrace(ts);
	}

	/* And enter syscall detection loop */
	tracee_error_t rc;

	/* And enter syscall detection loop */
	rc = tracee_s_tracksyscalls_ptrace(ts);

	if(rc != TRACEE_SUCCESS)
	{
		return rc;
	}

	while(ts->tracked)
	{
		int has_bit_7 = 0;

		rc = tracee_s_wait_for_sig_ptrace(ts, SIGTRAP, &has_bit_7);

		if(rc != TRACEE_SUCCESS)
		{
			break;
		}

		/* Could be a sigtrap to clear us out */
		if(!has_bit_7)
		{
			/* Skip non SYSCALL TRAPS */
			if(ts->tracked == 0)
			{
				break;
			}

			rc = tracee_s_tracksyscalls_ptrace(ts);

			if(rc != TRACEE_SUCCESS)
			{
				break;
			}
			continue;
		}

		/* Syscall entry */
		double start = now();

		errno = 0;

        scall_enter_desc_t * enter = scal_decode_enter(ts->pid);

		/* Continue */
		rc = tracee_s_tracksyscalls_ptrace(ts);

		if(rc != TRACEE_SUCCESS)
		{
			break;
		}

		/* Syscall exit */
		rc = tracee_s_wait_for_sig_ptrace(ts, SIGTRAP, &has_bit_7);

		if(rc != TRACEE_SUCCESS)
		{
			break;
		}

		/* Could be a sigtrap to clear us out */
		if(!has_bit_7)
		{
			if(ts->tracked == 0)
			{
				break;
			}
		}

		double end = now();

		errno = 0;

		scall_exit_desc_t * exit = NULL;

		if(enter)
		{
			exit = scal_decode_exit(ts->pid, enter->scall_id);
		}

		/* Continue */
		rc = tracee_s_tracksyscalls_ptrace(ts);

		if(rc != TRACEE_SUCCESS)
		{
			break;
		}

		/* Call the callback */
		int cbret = (ts->cb)(ts,
		                     ts->pid,
		                     enter,
							 exit,
		                     start,
		                     end - start,
		                     NULL,
		                     0);

        scal_free_enter(enter);
        scal_free_exit(exit);


		if(cbret)
		{
			break;
		}
	}

	if(!ts->running)
	{
		/* If running iterrupt first */
		rc = tracee_s_continue_ptrace(ts, 0);

		if(rc != TRACEE_SUCCESS)
		{
			return rc;
		}
	}


	ts->tracked = 0;

	return TRACEE_SUCCESS;
}

tracee_error_t tracee_s_track_ptrace(struct tracee_s *ts)
{
	if(ts->tracked)
	{
		return TRACEE_ERR_ALREADY_TRACKED;
	}

	if(!ts->cb)
	{
		return TRACEE_ERR_PARAM;
	}

	ts->tracked = 1;

	__ptrace_syscall_tracker(ts);

	return TRACEE_SUCCESS;
}

tracee_error_t tracee_s_untrack_ptrace(struct tracee_s *ts)
{
	if(!ts->tracked)
	{
		return TRACEE_ERR_PARAM;
	}


	ts->tracked = 0;
	kill(ts->pid, SIGTRAP);


	return TRACEE_SUCCESS;
}

/**********************************
* TRACEE INTERNAL IMPLEMENTATION *
**********************************/

tracee_error_t tracee_s_track(struct tracee_s *ts)
{
	if(!ts)
	{
		return TRACEE_ERR_PARAM;
	}

	tracee_error_t ret;

	switch(ts->kind)
	{
		case TRACEE_PTRACE:

			ret = tracee_s_track_ptrace(ts);

			break;

		default:
			abort();
	}


	return ret;
}

tracee_error_t tracee_s_untrack(struct tracee_s *ts)
{
	if(!ts)
	{
		return TRACEE_ERR_PARAM;
	}

	tracee_error_t ret;

	switch(ts->kind)
	{
		case TRACEE_PTRACE:

			ret = tracee_s_untrack_ptrace(ts);

			break;

		default:
			abort();
	}


	return ret;
}

tracee_error_t tracee_s_attach(struct tracee_s *ts)
{
	tracee_error_t ret;

	switch(ts->kind)
	{
		case TRACEE_PTRACE:

			ret = tracee_s_attach_ptrace(ts);

			break;

		default:
			abort();
	}


	return ret;
}

struct tracee_s *tracee_s_new(tracee_kind_t kind, pid_t pid, tracee_callback_t cb, tracee_error_t *err)
{
	struct tracee_s *ret = malloc(sizeof(struct tracee_s) );

	assert(ret != NULL);

	ret->pid  = pid;
	ret->kind = kind;
	ret->cb   = cb;

	tracee_error_t eret = tracee_s_attach(ret);

	if(eret != TRACEE_SUCCESS)
	{
		free(ret);

		if(err)
		{
			*err = eret;
		}

		return NULL;
	}

	return ret;
}

tracee_error_t tracee_s_end(struct tracee_s *ts)
{
	if(!ts)
	{
		return TRACEE_ERR_PARAM;
	}

	tracee_error_t ret;

	switch(ts->kind)
	{
		case TRACEE_PTRACE:

			ret = tracee_s_detach_ptrace(ts);

			break;

		default:
			abort();
	}


	return ret;
}

/***************************
* PUBLIC TRACEE INTERFACE *
***************************/

static void internal_init_once(void)
{
	static volatile int scall_init_done = 0;

	if(!scall_init_done)
	{
		scalls_init();
		scall_init_done = 1;
	}
}

tracee_t tracee_attach(pid_t pid, tracee_callback_t cb, tracee_error_t *err)
{
	internal_init_once();

	tracee_error_t   ierr;
	struct tracee_s *ret = tracee_s_new(TRACEE_PTRACE, pid, cb, &ierr);

	if(!ret)
	{
		if(err)
		{
			*err = ierr;
		}

		return TRACEE_NULL;
	}

	return (tracee_t)ret;
}

tracee_error_t tracee_track(tracee_t t)
{
	return tracee_s_track( (struct tracee_s *)t);
}

tracee_error_t tracee_untrack(tracee_t t)
{
	return tracee_s_untrack( (struct tracee_s *)t);
}

/**
 * @brief This deletes a process from monitoring
 *
 * @param t The tracee to detach from
 * @return tracee_error_t TRACEE_SUCCESS when all ok
 */
tracee_error_t tracee_detach(tracee_t t)
{
	return tracee_s_end( (struct tracee_s *)t);
}

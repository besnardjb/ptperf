#ifndef TRACEE_H
#define TRACEE_H

#include <sys/types.h>
#include <unistd.h>

#include "scalls.h"

/******************
* ERROR HANDLING *
******************/

typedef enum
{
	TRACEE_SUCCESS,
	TRACEE_ERR_NO_SUCH_PID,
	TRACEE_ERR_PARAM,
	TRACEE_ERR_PERM,
	TRACEE_ERR_UNREACHABLE,
	TRACEE_ERR_EXITED,
	TRACEE_ERR_OTHER,
	TRACEE_ERR_ALREADY_TRACKED,
	TRACEE_ERR_COUNT
}tracee_error_t;

static const char *const tracee_error_str[TRACEE_ERR_COUNT] = {
	"TRACEE_SUCCESS",
	"TRACEE_ERR_NO_SUCH_PID",
	"TRACEE_ERR_PARAM",
	"TRACEE_ERR_PERM",
	"TRACEE_ERR_UNREACHABLE",
	"TRACEE_ERR_EXITED",
	"TRACEE_ERR_OTHER",
	"TRACEE_ERR_ALREADY_TRACKED"
};


/********************
* TYPE DEFINITIONS *
********************/

/**
 * @brief This describes a tracked process
 *
 */
typedef void *tracee_t;

#define TRACEE_NULL    ( (tracee_t)NULL)

/***************************
* ATTACH DETACH INTERFACE *
***************************/


typedef int (*tracee_callback_t)(tracee_t tr,
                                 pid_t pid,
                                 scall_enter_desc_t *enter_desc,
                                 scall_exit_desc_t *exit_desc,
                                 double start_time,
                                 double duration,
                                 void *stack[],
                                 int stack_len);



/**
 * @brief This adds a new process for monitoring
 *
 * @param pid pid to monitor
 * @param err error explaining the TRACEE_NULL
 * @return tracee_t new tracee or TRACEE_NULL in case of error
 */
tracee_t tracee_attach(pid_t pid, tracee_callback_t cb, tracee_error_t *err);


tracee_error_t tracee_track(tracee_t t);

tracee_error_t tracee_untrack(tracee_t t);


/**
 * @brief This deletes a process from monitoring
 *
 * @param t The tracee to detach from
 * @return tracee_error_t TRACEE_SUCCESS when all ok
 */
tracee_error_t tracee_detach(tracee_t t);



#endif /* TRACEE_H */

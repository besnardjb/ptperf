#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/ptrace.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "conf.h"
#include "tracee.h"


int io_only = 0;

int safe_write_fd(int fd, void *buff, size_t len)
{
	size_t left_to_write = len;

	do
	{
		errno = 0;
		int ret = write(fd, buff, left_to_write);

		if(ret < 0)
		{
			if( (errno == EAGAIN) || (errno == EINTR) )
			{
				continue;
			}

			perror("write");

			return ret;
		}

		left_to_write -= ret;
	} while(left_to_write);

	return 0;
}

int syscall_cb_out_fd_text(tracee_t tracee,
                           pid_t pid,
                           scall_enter_desc_t *enter,
                           scall_exit_desc_t *exit,
                           double start_time,
                           double duration,
                           void *stack[],
                           int stack_len)
{
	if(!scall_is_io(enter->scall_id) && tau_perf_conf_get()->io_only)
	{
		return 0;
	}

	char enter_buff[512];
	scal_print_enter(enter, enter_buff, 512);

	char exit_buff[512];
	scal_print_exit(exit, exit_buff, 512);



	char buff[2048];

	snprintf(buff, 2048, "[%d] %s %s  [%.32g for %g]\n", pid, exit_buff, enter_buff, start_time, duration);

	return safe_write_fd(tau_perf_conf_get()->output_fd, buff, strlen(buff) );
}

int syscall_cb_out_fd_json(tracee_t tracee,
                           pid_t pid,
                           scall_enter_desc_t *enter,
                           scall_exit_desc_t *exit,
                           double start_time,
                           double duration,
                           void *stack[],
                           int stack_len)
{
	if(!scall_is_io(enter->scall_id) && tau_perf_conf_get()->io_only)
	{
		return 0;
	}


	char scallb[1024];
	scal_format_json(enter, exit, scallb, 2048);

	char buff[2048];
	snprintf(buff, 2048, "{ \"start\" : %.17g , \"duration\" : %.17g , \"syscall\" : %s }", start_time, duration, scallb);

	/* Note, we write the \0 to ease parsing */
	return safe_write_fd(tau_perf_conf_get()->output_fd, buff, strlen(buff) + 1);
}

int attach(pid_t pid)
{
	tracee_error_t err;

	tracee_callback_t cb = syscall_cb_out_fd_text;

	if(tau_perf_conf_get()->json)
	{
		cb = syscall_cb_out_fd_json;
	}


	tracee_t tc = tracee_attach(pid, cb, &err);

	if(tc == NULL)
	{
		printf("%s", tracee_error_str[err]);
		return 1;
	}

	tracee_track(tc);

	close(tau_perf_conf_get()->output_fd);

	tracee_detach(tc);

	return 0;
}

int dofork(char *myargv[])
{
	pid_t rpid = fork();

	if(rpid == 0)
	{
		/* Child */
		ptrace(PTRACE_TRACEME, 0, NULL, NULL);
		int rc = execvp(myargv[0], myargv);

		if(rc < 0)
		{
			perror("execvp");
			return 1;
		}
	}
	else
	{
		/* Parent */
		return attach(rpid);
	}


	return 0;
}

int check_fd(int fd)
{
	int fdup = dup(fd);

	if(fdup < 0)
	{
		perror("-r option file descriptor");
		return 1;
	}

	FILE *tf = fdopen(fdup, "w");

	if(!tf)
	{
		perror("-r option file descriptor");
		return 1;
	}

	fclose(tf);

	return 0;
}

void help(void)
{
	printf("perf [-h] [-j] [-o FILE] [-r FD] [-i] [-p PID]\n");
	printf("\n");
	printf("-p PID  : attach to PID\n");
	printf("-o FILE : store output in FILE (not compatible with -r)\n");
	printf("-r FD   : redirect output to FD (not compatible with -o)\n");
	printf("-i      : only show 'io' events\n");
	printf("-j      : use a JSON event output\n");
	printf("-h      : show this help\n");
}

int main(int argc, char **argv)
{
	char *cpid = NULL;

	int c;

	opterr = 0;

	while( (c = getopt(argc, argv, "hip:r:jo:") ) != -1)
	{
		switch(c)
		{
			case 'h':
				help();
				return 1;

			case 'i':
				tau_perf_conf_get()->io_only = 1;
				break;

			case 'j':
				tau_perf_conf_get()->json = 1;
				break;

			case 'o':
				if(tau_perf_conf_get()->output_fd != 0)
				{
					fprintf(stderr, "Cannot set both an output fd and an output file\n");
					return 1;
				}

				snprintf(tau_perf_conf_get()->outfile, 1024, "%s", optarg);

				tau_perf_conf_get()->output_fd = open(tau_perf_conf_get()->outfile, O_CREAT | O_WRONLY | O_EXCL, S_IRUSR | S_IWUSR);

				if(tau_perf_conf_get()->output_fd < 0)
				{
					perror("Output file (-o)");
					return 1;
				}

				break;

			case 'p':
				cpid = optarg;
				break;

			case 'r':
			{
				if(strlen(tau_perf_conf_get()->outfile) )
				{
					fprintf(stderr, "Cannot set both an output fd and an output file\n");
					return 1;
				}


				int fd = -1;
				fd = atoi(optarg);
				if(check_fd(fd) )
				{
					return 1;
				}
				else
				{
					tau_perf_conf_get()->output_fd = fd;
				}
				break;
			}

			case '?':
				if( (optopt == 'p') || (optopt == 'f') || (optopt == 'o') )
				{
					fprintf(stderr, "Option -%c requires an argument.\n", optopt);
				}
				else
				{
					fprintf(stderr, "Unknown option `-%c'.\n", optopt);
				}

				return 1;

			default:
				abort();
		}
	}



	if(!cpid)
	{
		int myargc = argc - optind;

		if(myargc == 0)
		{
			printf("Either attach to a pid with -p or provide a command to run\n");
			return 1;
		}

		char **myargv = malloc(sizeof(char *) * (myargc + 1) );

		if(!myargc)
		{
			perror("malloc");
			return 1;
		}

		int i;
		int cnt = 0;

		for(i = optind; i < argc; i++)
		{
			myargv[cnt] = strdup(argv[i]);
			//printf("%s\n", argv[i]);
			cnt++;
		}

		myargv[cnt] = NULL;

		return dofork(myargv);
	}
	else
	{
		pid_t pid = atoi(cpid);
		return attach(pid);
	}

	return 0;
}

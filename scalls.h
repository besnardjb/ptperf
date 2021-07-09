#ifndef SCALLS_H
#define SCALLS_H

#include <sys/types.h>

void scalls_init();

const char * scall_name(int id);

int scall_is_io(int id);

/************************
 * SYSCALL ENTER DECODE *
 ************************/

typedef struct
{
    char name[32];
    int scall_id;

    union
    {
        struct
        {
            char fname[512];
            int flags;
            int mode;
        }open;

        struct
        {
            int dfd;
            char fname[512];
            int flags;
            int mode;
        }openat;

        struct
        {
            int fd;
            void *buff;
            size_t count;
        }read_write;

        struct
        {
            int i;
        }oneint;

        struct
        {
            char fname[512];
            void *statbuf;
        }stat;

        struct
        {
            char fname[512];
            int mode;
        }access;

        struct
        {
            int fd;
            void *statbuf;
        }fstat;

    }args;

}scall_enter_desc_t;


scall_enter_desc_t * scal_decode_enter(pid_t pid);
char * scal_print_enter(scall_enter_desc_t *enter, char *buff, int size);
int scal_free_enter(scall_enter_desc_t *enter);

/***********************
 * SYSCALL EXIT DECODE *
 ***********************/

typedef struct
{
    int scall_id;

    union
    {
        struct
        {
            int i;
        }oneint;

        struct
        {
            ssize_t s;
        }ssizet;
    }args;

}scall_exit_desc_t;

scall_exit_desc_t *scal_decode_exit(pid_t pid, int syscall);
int scal_free_exit(scall_exit_desc_t *dexit);
char *scal_print_exit(scall_exit_desc_t *dexit, char *buff, int size);

/************************
 * JSON EVENT FORMATING *
 ************************/

char * scal_format_json(scall_enter_desc_t * enter, scall_exit_desc_t * exit, char *buf, int len);



#endif /* SCALLS_H */
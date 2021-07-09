#ifndef CONF_H
#define CONF_H

typedef struct tau_perf_conf_s
{
    int io_only;
    int output_fd;
    int json;
    char outfile[1024];
}tau_perf_conf;


tau_perf_conf * tau_perf_conf_get();

void tau_perf_conf_defaults();

#endif /* CONF_H */
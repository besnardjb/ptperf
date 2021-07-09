#include "conf.h"

#include <unistd.h>

tau_perf_conf __conf = { 0 };

void tau_perf_conf_defaults()
{
    __conf.output_fd = STDOUT_FILENO;
}

tau_perf_conf * tau_perf_conf_get()
{
    return &__conf;
}
#include <stdio.h>
#include <sys/resource.h>

int main() {
    struct rlimit lim;
    getrlimit(RLIMIT_STACK, &lim);
    printf("stack size: %ld\n", lim.rlim_cur);
    struct rlimit lim2;
    getrlimit(RLIMIT_NPROC, &lim2);
    printf("process limit: %ld\n", lim2.rlim_cur);
    struct rlimit lim3;
    getrlimit(RLIMIT_NOFILE, &lim3);
    printf("max file descriptors: %ld\n", lim3.rlim_cur);
    return 0;
}

#include <stdio.h>
#include <unistd.h>
#include <metadata/cpd.h>

void fnc_log(int level, const char *fmt, ...) {
    va_list vl;
    va_start(vl, fmt);
    fprintf(stderr, "%2d: ", level);
    vfprintf(stderr, fmt, vl);
    fprintf(stderr, "\n");
    va_end(vl);
}

int main (void) {
    pthread_t server;
    pthread_create(&server, NULL, cpd_server, NULL);

    while(1) {
	sleep(1);
    }
}

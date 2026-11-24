#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

/* Модифікований exec_basic: замість echo запускає ./new_program,
 * яка також друкує свій PID. */
int main(void)
{
    printf("Before exec: PID=%d\n", getpid());
    fflush(stdout);

    execl("./new_program", "new_program", (char *)NULL);

    perror("execl");

    return EXIT_FAILURE;
}

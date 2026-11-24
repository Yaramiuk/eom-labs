#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

/*
 * process_runner PROGRAM [ARGS...]
 *
 * Створює child через fork(), у child запускає PROGRAM через execvp(),
 * а parent чекає на завершення через waitpid() і аналізує status.
 */
int main(int argc, char *argv[])
{
    if (argc < 2) {
        fprintf(stderr, "Usage: %s PROGRAM [ARGS...]\n", argv[0]);
        return EXIT_FAILURE;
    }

    printf("Parent PID: %d\n", getpid());
    printf("Creating child...\n");
    fflush(stdout);              /* щоб буфер не продублювався в child */

    pid_t pid = fork();

    if (pid < 0) {
        perror("fork");
        return EXIT_FAILURE;
    }

    if (pid == 0) {
        /* ---- child ---- */
        printf("\nChild PID: %d\n", getpid());
        printf("Child PPID: %d\n", getppid());
        printf("Executing: %s\n\n", argv[1]);
        fflush(stdout);

        execvp(argv[1], &argv[1]);

        /* сюди потрапляємо лише якщо exec не вдався */
        fprintf(stderr, "exec %s: %s\n", argv[1], strerror(errno));
        _exit(127);              /* як shell: 127 = command not found */
    }

    /* ---- parent ---- */
    printf("Parent: created child %d\n", pid);
    fflush(stdout);

    int status;
    pid_t done;

    do {
        done = waitpid(pid, &status, 0);
    } while (done < 0 && errno == EINTR);

    if (done < 0) {
        perror("waitpid");
        return EXIT_FAILURE;
    }

    printf("\nChild %d completed\n", done);

    if (WIFEXITED(status)) {
        printf("Exit status: %d\n", WEXITSTATUS(status));
    } else if (WIFSIGNALED(status)) {
        printf("Terminated by signal %d (%s)\n",
               WTERMSIG(status), strsignal(WTERMSIG(status)));
    } else {
        printf("Child ended in an unexpected way (status=0x%x)\n", status);
    }

    printf("\nParent continues\n");

    return EXIT_SUCCESS;
}

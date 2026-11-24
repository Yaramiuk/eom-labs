#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

/*
 *   ./signal_parent_child          — child має default disposition SIGTERM
 *   ./signal_parent_child handler  — child встановлює власний handler SIGTERM
 */

static volatile sig_atomic_t got_sigterm = 0;

static void handle_sigterm(int signal_number)
{
    (void)signal_number;

    got_sigterm = 1;

    const char message[] = "Child: SIGTERM caught by handler, cleaning up\n";
    write(STDOUT_FILENO, message, sizeof(message) - 1);
}

int main(int argc, char *argv[])
{
    int use_handler = (argc > 1 && strcmp(argv[1], "handler") == 0);

    printf("Mode: %s\n", use_handler ? "custom SIGTERM handler" : "default SIGTERM");
    fflush(stdout);

    pid_t pid = fork();

    if (pid == -1) {
        perror("fork");
        return EXIT_FAILURE;
    }

    if (pid == 0) {
        if (use_handler) {
            struct sigaction action = {0};
            action.sa_handler = handle_sigterm;
            sigemptyset(&action.sa_mask);

            if (sigaction(SIGTERM, &action, NULL) == -1) {
                perror("sigaction");
                _exit(EXIT_FAILURE);
            }
        }

        printf("Child %d: waiting for signal...\n", getpid());
        fflush(stdout);

        while (!got_sigterm) {
            pause();
        }

        printf("Child: exiting normally with status 7\n");
        fflush(stdout);
        _exit(7);
    }

    sleep(2);

    printf("Parent: sending SIGTERM to child %d\n", pid);
    fflush(stdout);

    if (kill(pid, SIGTERM) == -1) {
        perror("kill");
    }

    int status;

    if (waitpid(pid, &status, 0) == -1) {
        perror("waitpid");
        return EXIT_FAILURE;
    }

    if (WIFSIGNALED(status)) {
        printf("Parent: child was terminated by signal %d (%s)\n",
               WTERMSIG(status), strsignal(WTERMSIG(status)));
    } else if (WIFEXITED(status)) {
        printf("Parent: child exited normally, status %d\n", WEXITSTATUS(status));
    }

    return EXIT_SUCCESS;
}

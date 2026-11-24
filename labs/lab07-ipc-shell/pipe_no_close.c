#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

/*
 * Експеримент: child читає pipe до EOF.
 *
 *   ./pipe_no_close          — child НЕ закриває свою копію write end:
 *                              після отримання даних read() блокується назавжди,
 *                              бо writer (сам child) ще існує → EOF не настає.
 *   ./pipe_no_close close    — child закриває unused write end:
 *                              після закриття write end у parent read() == 0 (EOF).
 */
int main(int argc, char *argv[])
{
    int close_unused = (argc > 1 && strcmp(argv[1], "close") == 0);

    int pipefd[2];

    if (pipe(pipefd) == -1) {
        perror("pipe");
        return EXIT_FAILURE;
    }

    pid_t pid = fork();

    if (pid == -1) {
        perror("fork");
        return EXIT_FAILURE;
    }

    if (pid == 0) {
        /* Child reads until EOF */
        if (close_unused) {
            close(pipefd[1]);
        }

        char buffer[128];
        ssize_t n;
        size_t total = 0;

        while ((n = read(pipefd[0], buffer, sizeof(buffer))) > 0) {
            total += (size_t)n;
            printf("Child: got %zd bytes: %.*s", n, (int)n, buffer);
            fflush(stdout);
        }

        if (n == -1) {
            perror("read");
            _exit(EXIT_FAILURE);
        }

        printf("Child: EOF reached, total %zu bytes\n", total);

        close(pipefd[0]);
        _exit(EXIT_SUCCESS);
    }

    /* Parent writes */
    close(pipefd[0]);

    const char *lines[] = { "line 1\n", "line 2\n", "line 3\n" };

    for (int i = 0; i < 3; i++) {
        if (write(pipefd[1], lines[i], strlen(lines[i])) == -1) {
            perror("write");
        }
    }

    close(pipefd[1]);                 /* parent завжди закриває свій write end */
    printf("Parent: closed write end, waiting for child %d...\n", pid);
    fflush(stdout);

    waitpid(pid, NULL, 0);

    printf("Parent: child finished\n");

    return EXIT_SUCCESS;
}

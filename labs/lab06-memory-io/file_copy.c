#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define BUFFER_SIZE 4096

/*
 * Записує рівно count байтів з buffer у fd, повторюючи write()
 * при partial write та при перериванні сигналом (EINTR).
 * Повертає 0 при успіху, -1 при помилці (errno встановлено).
 */
static int write_all(int fd, const char *buffer, size_t count)
{
    size_t total_written = 0;

    while (total_written < count) {
        ssize_t written = write(fd,
                                buffer + total_written,
                                count - total_written);

        if (written == -1) {
            if (errno == EINTR) {
                continue;
            }
            return -1;
        }

        total_written += (size_t)written;
    }

    return 0;
}

int main(int argc, char *argv[])
{
    if (argc != 3) {
        fprintf(stderr,
                "Usage: %s <source> <destination>\n",
                argv[0]);
        return EXIT_FAILURE;
    }

    int source = open(argv[1], O_RDONLY);

    if (source == -1) {
        fprintf(stderr, "open source '%s': %s\n", argv[1], strerror(errno));
        return EXIT_FAILURE;
    }

    int destination = open(argv[2],
                           O_WRONLY | O_CREAT | O_TRUNC,
                           0644);

    if (destination == -1) {
        fprintf(stderr, "open destination '%s': %s\n", argv[2], strerror(errno));
        close(source);              /* не залишаємо source відкритим */
        return EXIT_FAILURE;
    }

    char buffer[BUFFER_SIZE];
    ssize_t bytes_read;
    int status = EXIT_SUCCESS;

    for (;;) {
        bytes_read = read(source, buffer, sizeof(buffer));

        if (bytes_read == 0) {
            break;                  /* EOF */
        }

        if (bytes_read == -1) {
            if (errno == EINTR) {
                continue;
            }
            perror("read");
            status = EXIT_FAILURE;
            break;
        }

        if (write_all(destination, buffer, (size_t)bytes_read) == -1) {
            perror("write");
            status = EXIT_FAILURE;
            break;
        }
    }

    if (close(source) == -1) {
        perror("close source");
        status = EXIT_FAILURE;
    }

    /* close() destination може повідомити про відкладену помилку запису */
    if (close(destination) == -1) {
        perror("close destination");
        status = EXIT_FAILURE;
    }

    return status;
}

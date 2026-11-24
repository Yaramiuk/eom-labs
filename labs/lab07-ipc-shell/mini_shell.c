/*
 * mini_shell — навчальна командна оболонка (Lab 7).
 *
 * Підтримує:
 *   - зовнішні команди:        ls -l /tmp
 *   - built-in:                cd [dir], exit [status]
 *   - перенаправлення stdout:  echo hello > output.txt
 *   - одну pipe:               ls -l | wc -l
 *   - pipe + перенаправлення:  ls -l | wc -l > count.txt
 *   - Ctrl+C перериває foreground-команду, але не сам shell
 *
 * Не підтримує (поза CORE): лапки, змінні, glob, ;, &&, ||, &, >>, <, кілька pipes.
 *
 * Використовуються лише: fork, execvp, waitpid, pipe, dup2, open, close,
 * chdir, sigaction. Без system() і без сторонніх парсерів.
 */

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#define MAX_LINE 1024
#define MAX_ARGS 64

/* Розібрана команда: до двох частин pipeline і необов'язковий файл для '>' */
typedef struct {
    char *left[MAX_ARGS];     /* argv першої (або єдиної) команди, NULL-terminated */
    char *right[MAX_ARGS];    /* argv другої команди після '|', або right[0] == NULL */
    char *outfile;            /* файл після '>', або NULL */
    int   has_pipe;
} Command;

/* ---------- signals ---------- */

/* Shell ігнорує SIGINT: Ctrl+C отримує вся foreground process group,
   але вбиває лише дочірні процеси, а shell продовжує працювати. */
static void shell_ignore_signals(void)
{
    struct sigaction action = {0};
    action.sa_handler = SIG_IGN;
    sigemptyset(&action.sa_mask);
    sigaction(SIGINT, &action, NULL);
    sigaction(SIGQUIT, &action, NULL);
}

/* Child перед exec() відновлює стандартну поведінку сигналів.
   (Ігноровані сигнали успадковуються через exec, тому це обов'язково.) */
static void child_restore_signals(void)
{
    struct sigaction action = {0};
    action.sa_handler = SIG_DFL;
    sigemptyset(&action.sa_mask);
    sigaction(SIGINT, &action, NULL);
    sigaction(SIGQUIT, &action, NULL);
}

/* ---------- parsing ---------- */

/* Додає пробіли навколо '|' і '>', щоб "ls|wc" і "echo hi>f" теж працювали. */
static int space_operators(const char *in, char *out, size_t out_size)
{
    size_t j = 0;

    for (size_t i = 0; in[i] != '\0'; i++) {
        int op = (in[i] == '|' || in[i] == '>');
        size_t need = op ? 3 : 1;

        if (j + need >= out_size) {
            return -1;
        }

        if (op) {
            out[j++] = ' ';
            out[j++] = in[i];
            out[j++] = ' ';
        } else {
            out[j++] = in[i];
        }
    }

    out[j] = '\0';
    return 0;
}

/* Повертає 0 — успіх, 1 — порожній рядок, -1 — синтаксична помилка. */
static int parse_line(char *line, Command *cmd)
{
    memset(cmd, 0, sizeof(*cmd));

    char **current = cmd->left;
    int argc = 0;
    int expect_file = 0;

    for (char *tok = strtok(line, " \t"); tok != NULL; tok = strtok(NULL, " \t")) {

        if (expect_file) {
            if (strcmp(tok, "|") == 0 || strcmp(tok, ">") == 0) {
                fprintf(stderr, "myshell: syntax error near '%s'\n", tok);
                return -1;
            }
            cmd->outfile = tok;
            expect_file = 0;
            continue;
        }

        if (strcmp(tok, "|") == 0) {
            if (cmd->has_pipe) {
                fprintf(stderr, "myshell: only one pipe is supported\n");
                return -1;
            }
            if (argc == 0) {
                fprintf(stderr, "myshell: syntax error: missing command before '|'\n");
                return -1;
            }
            if (cmd->outfile != NULL) {
                fprintf(stderr, "myshell: '>' is only supported at the end of the line\n");
                return -1;
            }
            current[argc] = NULL;
            cmd->has_pipe = 1;
            current = cmd->right;
            argc = 0;
            continue;
        }

        if (strcmp(tok, ">") == 0) {
            if (cmd->outfile != NULL) {
                fprintf(stderr, "myshell: only one '>' is supported\n");
                return -1;
            }
            expect_file = 1;
            continue;
        }

        if (argc >= MAX_ARGS - 1) {
            fprintf(stderr, "myshell: too many arguments\n");
            return -1;
        }
        current[argc++] = tok;
    }

    current[argc] = NULL;

    if (expect_file) {
        fprintf(stderr, "myshell: syntax error: missing file name after '>'\n");
        return -1;
    }

    if (!cmd->has_pipe && cmd->left[0] == NULL) {
        if (cmd->outfile != NULL) {
            fprintf(stderr, "myshell: syntax error: missing command\n");
            return -1;
        }
        return 1;                       /* порожній рядок */
    }

    if (cmd->has_pipe && cmd->right[0] == NULL) {
        fprintf(stderr, "myshell: syntax error: missing command after '|'\n");
        return -1;
    }

    return 0;
}

/* ---------- child helpers (виконуються лише у child) ---------- */

/* Перенаправляє stdout у файл: open → dup2 → close. */
static void child_redirect_stdout(const char *path)
{
    int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644);

    if (fd == -1) {
        fprintf(stderr, "myshell: %s: %s\n", path, strerror(errno));
        _exit(EXIT_FAILURE);
    }

    if (dup2(fd, STDOUT_FILENO) == -1) {
        perror("myshell: dup2");
        _exit(EXIT_FAILURE);
    }

    close(fd);
}

static void child_exec(char **argv)
{
    child_restore_signals();

    execvp(argv[0], argv);

    /* сюди потрапляємо лише при помилці exec */
    fprintf(stderr, "myshell: %s: %s\n", argv[0], strerror(errno));
    _exit(errno == ENOENT ? 127 : 126);
}

/* ---------- parent helpers ---------- */

/* waitpid з повтором при EINTR; повертає status або -1. */
static int wait_child(pid_t pid)
{
    int status;

    while (waitpid(pid, &status, 0) == -1) {
        if (errno != EINTR) {
            perror("myshell: waitpid");
            return -1;
        }
    }

    return status;
}

/* Повідомити, якщо команду перервано сигналом (Ctrl+C → новий рядок). */
static void report_status(int status)
{
    if (status == -1) {
        return;
    }

    if (WIFSIGNALED(status)) {
        int sig = WTERMSIG(status);
        if (sig == SIGINT) {
            fputc('\n', stderr);
        } else {
            fprintf(stderr, "myshell: terminated by signal %d (%s)\n",
                    sig, strsignal(sig));
        }
    }
}

/* ---------- execution ---------- */

static void run_simple(Command *cmd)
{
    pid_t pid = fork();

    if (pid == -1) {
        perror("myshell: fork");
        return;
    }

    if (pid == 0) {
        if (cmd->outfile != NULL) {
            child_redirect_stdout(cmd->outfile);
        }
        child_exec(cmd->left);
    }

    report_status(wait_child(pid));
}

static void run_pipeline(Command *cmd)
{
    int pipefd[2];

    if (pipe(pipefd) == -1) {
        perror("myshell: pipe");
        return;
    }

    pid_t child1 = fork();

    if (child1 == -1) {
        perror("myshell: fork");
        close(pipefd[0]);
        close(pipefd[1]);
        return;
    }

    if (child1 == 0) {
        /* left: stdout → pipe write end */
        if (dup2(pipefd[1], STDOUT_FILENO) == -1) {
            perror("myshell: dup2");
            _exit(EXIT_FAILURE);
        }
        close(pipefd[0]);
        close(pipefd[1]);
        child_exec(cmd->left);
    }

    pid_t child2 = fork();

    if (child2 == -1) {
        perror("myshell: fork");
        close(pipefd[0]);
        close(pipefd[1]);          /* left отримає EOF/SIGPIPE і завершиться */
        wait_child(child1);
        return;
    }

    if (child2 == 0) {
        /* right: stdin ← pipe read end, stdout → файл (якщо є '>') */
        if (dup2(pipefd[0], STDIN_FILENO) == -1) {
            perror("myshell: dup2");
            _exit(EXIT_FAILURE);
        }
        close(pipefd[0]);
        close(pipefd[1]);

        if (cmd->outfile != NULL) {
            child_redirect_stdout(cmd->outfile);
        }
        child_exec(cmd->right);
    }

    /* Parent не використовує pipe: закрити обидва кінці, інакше right ніколи не побачить EOF */
    close(pipefd[0]);
    close(pipefd[1]);

    int status1 = wait_child(child1);
    int status2 = wait_child(child2);

    /* щоб не друкувати два порожні рядки після Ctrl+C */
    if (WIFSIGNALED(status2) || status2 == -1) {
        report_status(status2);
    } else {
        report_status(status1);
    }
}

/* ---------- built-ins ---------- */

/* Повертає 1, якщо команда — built-in і її виконано. */
static int run_builtin(Command *cmd, int *should_exit, int *exit_status)
{
    char **argv = cmd->left;

    if (cmd->has_pipe) {
        return 0;                       /* built-ins у pipeline не підтримуються */
    }

    if (strcmp(argv[0], "exit") == 0) {
        *should_exit = 1;
        *exit_status = (argv[1] != NULL) ? atoi(argv[1]) : EXIT_SUCCESS;
        return 1;
    }

    if (strcmp(argv[0], "cd") == 0) {
        const char *dir = argv[1];

        if (dir == NULL) {
            dir = getenv("HOME");
            if (dir == NULL) {
                fprintf(stderr, "myshell: cd: HOME not set\n");
                return 1;
            }
        }

        if (argv[1] != NULL && argv[2] != NULL) {
            fprintf(stderr, "myshell: cd: too many arguments\n");
            return 1;
        }

        if (chdir(dir) == -1) {
            fprintf(stderr, "myshell: cd: %s: %s\n", dir, strerror(errno));
        }
        return 1;
    }

    return 0;
}

/* ---------- main loop ---------- */

int main(void)
{
    char line[MAX_LINE];
    char spaced[MAX_LINE * 3];
    int should_exit = 0;
    int exit_status = EXIT_SUCCESS;

    shell_ignore_signals();

    while (!should_exit) {
        printf("myshell> ");
        fflush(stdout);

        if (fgets(line, sizeof(line), stdin) == NULL) {
            if (ferror(stdin) && errno == EINTR) {
                clearerr(stdin);
                continue;
            }
            putchar('\n');              /* Ctrl+D (EOF) — вихід */
            break;
        }

        size_t len = strlen(line);

        if (len > 0 && line[len - 1] == '\n') {
            line[len - 1] = '\0';
        } else if (len == sizeof(line) - 1) {
            fprintf(stderr, "myshell: line too long\n");
            int c;
            while ((c = getchar()) != '\n' && c != EOF) {
                /* відкинути залишок рядка */
            }
            continue;
        }

        if (space_operators(line, spaced, sizeof(spaced)) == -1) {
            fprintf(stderr, "myshell: line too long\n");
            continue;
        }

        Command cmd;

        if (parse_line(spaced, &cmd) != 0) {
            continue;                   /* порожній рядок або синтаксична помилка */
        }

        if (run_builtin(&cmd, &should_exit, &exit_status)) {
            continue;
        }

        if (cmd.has_pipe) {
            run_pipeline(&cmd);
        } else {
            run_simple(&cmd);
        }
    }

    return exit_status;
}

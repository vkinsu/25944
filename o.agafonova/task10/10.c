#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Использование: %s команда [аргументы...]\n", argv[0]);
        return 1;
    }

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        return 1;
    }
    if (pid == 0) {
        execvp(argv[1], &argv[1]);
        perror("execvp");
        _exit(127);
    }

    int status;
    if (waitpid(pid, &status, 0) == -1) {
        perror("waitpid");
        return 1;
    }
    if (WIFEXITED(status))
        printf("Код завершения: %d\n", WEXITSTATUS(status));
    else if (WIFSIGNALED(status))
        printf("Процесс завершён сигналом: %d\n", WTERMSIG(status));

    return 0;
}
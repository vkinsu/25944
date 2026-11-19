#define _POSIX_C_SOURCE 200809L

#include <errno.h>    // errno, EINTR
#include <stdio.h>    // printf, fprintf, perror
#include <sys/wait.h> // waitpid и проверка состояния потомка
#include <unistd.h>   // fork, execvp, _exit

int main(int argc, char *argv[])
{
    pid_t child;
    pid_t result;
    int status;

    if (argc < 2 || argv[1][0] == '\0')
    {
        fprintf(stderr, "Usage: %s command [args...]\n", argv[0]);
        return 1;
    }

    child = fork();
    if (child == -1)
    {
        perror("fork");
        return 1;
    }

    if (child == 0)
    {
        execvp(argv[1], &argv[1]); // запускаем команду со всеми её аргументами
        perror("execvp");         // сюда попадаем только при ошибке запуска
        _exit(1);
    }

    do
    {
        result = waitpid(child, &status, 0); // ждём именно этого потомка
    }
    while (result == -1 && errno == EINTR);

    if (result == -1)
    {
        perror("waitpid");
        return 1;
    }

    if (WIFEXITED(status))
    {
        printf("Child exit code: %d\n", WEXITSTATUS(status));
    }
    else if (WIFSIGNALED(status))
    {
        printf("Child terminated by signal: %d\n", WTERMSIG(status));
    }

    return 0;
}
#define _POSIX_C_SOURCE 200809L // включаем объявления функций POSIX

#include <stdio.h> // printf, fprintf, perror, fflush
#include <string.h> // strcmp
#include <unistd.h> // fork, execvp, _exit
#include <sys/wait.h> // waitpid и проверка состояния потомка
#include <errno.h> // errno, EINTR, ENOENT

int main(int argc, char *argv[])
{
    pid_t child; // идентификатор дочернего процесса
    pid_t result; // результат ожидания потомка
    int status; // состояние завершившегося потомка
    int exit_code; // код, который вернёт родитель
    int exec_error; // сохраняем ошибку execvp до вызова perror

    if (argc == 2 && (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0))
    {
        printf("Usage: %s command [argument ...]\n", argv[0]);
        return 0;
    }
    if (argc < 2 || argv[1][0] == '\0') // команда должна быть указана непустой строкой
    {
        fprintf(stderr, "Usage: %s command [argument ...]\n", argv[0]);
        return 1;
    }

    child = fork(); // создаём процесс для выполнения команды
    if (child == -1)
    {
        perror("fork");
        return 1;
    }
    if (child == 0)
    {
        execvp(argv[1], &argv[1]); // передаём команду и все её аргументы без изменений
        exec_error = errno; // perror может изменить errno
        perror("execvp"); // execvp возвращается только при ошибке
        _exit(exec_error == ENOENT ? 127 : 126); // различаем отсутствие команды и невозможность её запуска
    }

    do
    {
        result = waitpid(child, &status, 0); // ждём завершения запущенной команды
    }
    while (result == -1 && errno == EINTR); // повторяем ожидание после прерывания сигналом
    if (result == -1)
    {
        perror("waitpid");
        return 1;
    }
    if (WIFEXITED(status)) // извлекаем код только при обычном завершении
    {
        exit_code = WEXITSTATUS(status);
        if (printf("Child exit code: %d\n", exit_code) < 0 || fflush(stdout) == EOF)
        {
            perror("stdout");
            return 1;
        }
        return exit_code; // родитель возвращает код выполненной команды
    }
    if (WIFSIGNALED(status)) // завершение сигналом сообщаем отдельно от кода выхода
    {
        if (printf("Child terminated by signal: %d\n", WTERMSIG(status)) < 0 || fflush(stdout) == EOF)
        {
            perror("stdout");
            return 1;
        }
        return 128 + WTERMSIG(status); // используем принятый в оболочках код завершения сигналом
    }
    return 1;
}

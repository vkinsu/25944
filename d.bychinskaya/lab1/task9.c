#define _POSIX_C_SOURCE 200809L // включаем объявления функций POSIX

#include <stdio.h> // printf, fprintf, perror, fflush
#include <string.h> // strcmp
#include <unistd.h> // fork, execlp, _exit
#include <sys/wait.h> // waitpid и проверка состояния потомка
#include <errno.h> // errno, EINTR

int main(int argc, char *argv[])
{
    pid_t child; // идентификатор дочернего процесса
    pid_t result; // результат ожидания потомка
    int status; // состояние завершившегося потомка
    int output_error = 0; // запоминаем ошибку вывода, чтобы всё равно дождаться потомка

    if (argc == 2 && (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0))
    {
        printf("Usage: %s file\n", argv[0]);
        return 0;
    }
    if (argc != 2 || argv[1][0] == '\0') // требуется один непустой путь к файлу
    {
        fprintf(stderr, "Usage: %s file\n", argv[0]);
        return 1;
    }

    child = fork(); // создаём дочерний процесс
    if (child == -1)
    {
        perror("fork");
        return 1;
    }
    if (child == 0)
    {
        execlp("cat", "cat", "--", argv[1], (char *)NULL); // передаём имя файла cat, даже если оно начинается с дефиса
        perror("execlp"); // сюда попадаем только при ошибке запуска cat
        _exit(127); // завершаем потомка без повторной очистки буферов родителя
    }

    if (printf("Parent: waiting for child.\n") < 0 || fflush(stdout) == EOF) // выводим текст родителя до ожидания
    {
        perror("stdout");
        output_error = 1;
    }
    do
    {
        result = waitpid(child, &status, 0); // ждём именно созданного потомка
    }
    while (result == -1 && errno == EINTR); // после прерывания сигналом повторяем ожидание
    if (result == -1)
    {
        perror("waitpid");
        return 1;
    }
    if (printf("Parent: child finished.\n") < 0 || fflush(stdout) == EOF) // последняя строка родителя появляется после завершения cat
    {
        perror("stdout");
        output_error = 1;
    }
    if (output_error)
    {
        return 1;
    }
    if (WIFEXITED(status)) // код выхода определён только при обычном завершении
    {
        return WEXITSTATUS(status);
    }
    if (WIFSIGNALED(status)) // при сигнале возвращаем принятый в оболочках код
    {
        return 128 + WTERMSIG(status);
    }
    return 1;
}

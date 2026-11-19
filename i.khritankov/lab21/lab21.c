#define _XOPEN_SOURCE 500 // включаем объявления функций POSIX

#include <errno.h> // errno, EINTR
#include <signal.h> // sigaction, sigprocmask, sigsuspend
#include <stdint.h> // SIG_ATOMIC_MAX
#include <stdio.h> // printf, fprintf, perror
#include <string.h> // strcmp
#include <unistd.h> // isatty, write

static volatile sig_atomic_t bells = 0; // число успешно записанных звонков
static volatile sig_atomic_t finished = 0; // сигнал SIGQUIT завершает ожидание
static volatile sig_atomic_t failed = 0; // ошибка записи или переполнение счётчика

static void handle_signal(int number) // обработчик использует только безопасные операции
{
    int saved_errno = errno; // сохраняем ошибку прерванного системного вызова
    ssize_t result;

    if (number == SIGQUIT)
    {
        finished = 1;
    }
    else if (number == SIGINT && !finished && !failed)
    {
        if (bells == SIG_ATOMIC_MAX)
        {
            failed = 1; // не допускаем переполнения знакового счётчика
        }
        else
        {
            do
            {
                result = write(STDOUT_FILENO, "\a", 1); // терминал получает символ звонка
            }
            while (result == -1 && errno == EINTR);
            if (result == 1)
            {
                bells++; // учитываем только действительно выведенный звонок
            }
            else
            {
                failed = 1;
            }
        }
    }
    errno = saved_errno; // обработчик не меняет errno основной программы
}

int main(int argc, char *argv[])
{
    struct sigaction action; // одинаковый обработчик для двух сигналов
    sigset_t blocked; // сигналы блокируются вне sigsuspend
    sigset_t previous; // маска, действовавшая до запуска ожидания
    sigset_t waiting; // маска на время атомарного ожидания

    if (argc == 2 && (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0))
    {
        printf("Usage: %s\nCtrl+C rings the terminal bell; Ctrl+\\ prints the count and exits.\n", argv[0]);
        return 0;
    }
    if (argc != 1)
    {
        fprintf(stderr, "Usage: %s [-h|--help]\n", argv[0]);
        return 1;
    }
    if (!isatty(STDIN_FILENO))
    {
        fprintf(stderr, "Standard input must be a terminal.\n");
        return 1; // пустой перенаправленный ввод не приводит к бесконечному ожиданию
    }

    sigemptyset(&blocked);
    sigaddset(&blocked, SIGINT);
    sigaddset(&blocked, SIGQUIT);
    if (sigprocmask(SIG_BLOCK, &blocked, &previous) == -1)
    {
        perror("sigprocmask");
        return 1;
    }
    action.sa_flags = 0; // не включаем дополнительные режимы обработки
    action.sa_handler = handle_signal;
    action.sa_mask = blocked; // обработчики не прерывают друг друга
    if (sigaction(SIGINT, &action, NULL) == -1 || sigaction(SIGQUIT, &action, NULL) == -1)
    {
        perror("sigaction");
        return 1;
    }
    waiting = previous;
    sigdelset(&waiting, SIGINT);
    sigdelset(&waiting, SIGQUIT);
    printf("Ctrl+C: bell; Ctrl+\\: count and exit.\n");
    if (fflush(stdout) == EOF)
    {
        perror("fflush");
        return 1;
    }
    while (!finished && !failed)
    {
        sigsuspend(&waiting); // проверка флага и разблокирование не оставляют гонки
    }
    if (failed)
    {
        fprintf(stderr, "Cannot emit or count another terminal bell.\n");
        return 1;
    }
    if (printf("\nBells: %ld\n", (long)bells) < 0 || fflush(stdout) == EOF)
    {
        perror("stdout");
        return 1;
    }
    return 0;
}

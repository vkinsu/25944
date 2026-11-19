#define _XOPEN_SOURCE 500 // включаем объявления функций POSIX

#include <ctype.h> // toupper
#include <errno.h> // errno, EINTR
#include <locale.h> // setlocale, LC_CTYPE
#include <signal.h> // sigaction, SIGPIPE, SIG_IGN
#include <stdio.h> // printf, fprintf, perror
#include <string.h> // strcmp
#include <sys/types.h> // pid_t
#include <sys/wait.h> // waitpid, WIFEXITED, WEXITSTATUS
#include <unistd.h> // pipe, fork, read, write, close, _exit
#include <wchar.h> // fgetwc, fputwc, WEOF
#include <wctype.h> // towupper

static int write_all(int descriptor, const char *buffer, size_t length) // записываем всю порцию данных
{
    ssize_t written;
    size_t offset = 0;

    while (offset < length)
    {
        written = write(descriptor, buffer + offset, length - offset);
        if (written == -1)
        {
            if (errno == EINTR)
            {
                continue; // повторяем запись после прерывания сигналом
            }
            return -1;
        }
        if (written == 0)
        {
            errno = EIO; // отсутствие продвижения не должно создавать бесконечный цикл
            return -1;
        }
        offset += (size_t)written;
    }
    return 0;
}

static int copy_text(int input, int output) // передаём текст порциями по 256 байт
{
    char buffer[256];
    ssize_t received;

    while (1)
    {
        received = read(input, buffer, sizeof(buffer));
        if (received == 0)
        {
            return 0; // пустой ввод и конец файла обрабатываются одинаково
        }
        if (received == -1)
        {
            if (errno == EINTR)
            {
                continue;
            }
            perror("read");
            return 1;
        }
        if (write_all(output, buffer, (size_t)received) == -1)
        {
            perror("write");
            return 1;
        }
    }
}

static int uppercase_text(int input) // ребёнок декодирует символы согласно текущей локали
{
    FILE *stream;
    wint_t character;
    int status = 0;

    stream = fdopen(input, "r"); // stdio собирает символы даже на границах порций канала
    if (stream == NULL)
    {
        perror("fdopen");
        close(input);
        return 1;
    }
    while (1)
    {
        character = fgetwc(stream);
        if (character == WEOF)
        {
            if (ferror(stream))
            {
                if (errno == EINTR)
                {
                    clearerr(stream);
                    continue;
                }
                perror("fgetwc");
                status = 1;
            }
            break;
        }
        if (character <= 127)
        {
            character = (wint_t)toupper((unsigned char)character); // ASCII преобразуем функцией из условия задания
        }
        else
        {
            character = towupper(character); // русские буквы преобразуются в локали UTF-8
        }
        if (fputwc((wchar_t)character, stdout) == WEOF)
        {
            perror("fputwc");
            status = 1;
            break;
        }
    }
    if (fclose(stream) == EOF) // закрываем поток и принадлежащий ему конец канала
    {
        perror("fclose");
        status = 1;
    }
    if (fflush(stdout) == EOF) // _exit не сбрасывает накопленный вывод автоматически
    {
        perror("fflush");
        status = 1;
    }
    return status;
}

int main(int argc, char *argv[])
{
    int descriptors[2]; // концы канала: чтение и запись
    pid_t child;
    int status = 0;
    int child_status;
    struct sigaction action;

    if (argc == 2 && (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0))
    {
        printf("Usage: %s\nRead text from standard input; the child prints it in uppercase.\n", argv[0]);
        return 0;
    }
    if (argc != 1)
    {
        fprintf(stderr, "Usage: %s [-h|--help]\n", argv[0]);
        return 1;
    }
    if (setlocale(LC_CTYPE, "") == NULL) // берём кодировку и правила регистра из окружения
    {
        fprintf(stderr, "Cannot select the character locale from the environment.\n");
        return 1;
    }
    action.sa_flags = 0; // не включаем дополнительные режимы обработки
    action.sa_handler = SIG_IGN;
    sigemptyset(&action.sa_mask);
    if (sigaction(SIGPIPE, &action, NULL) == -1)
    {
        perror("sigaction");
        return 1;
    }
    if (pipe(descriptors) == -1)
    {
        perror("pipe");
        return 1;
    }
    child = fork(); // дочерний процесс получает копии обоих дескрипторов
    if (child == -1)
    {
        perror("fork");
        close(descriptors[0]);
        close(descriptors[1]);
        return 1;
    }
    if (child == 0)
    {
        close(descriptors[1]); // ребёнок только читает из канала
        status = uppercase_text(descriptors[0]);
        _exit(status); // буферы stdio родителя повторно не сбрасываются
    }

    close(descriptors[0]); // родитель только пишет в канал
    status = copy_text(STDIN_FILENO, descriptors[1]);
    if (close(descriptors[1]) == -1)
    {
        perror("close");
        status = 1;
    }
    while (waitpid(child, &child_status, 0) == -1) // ждём ребёнка даже после ошибки записи
    {
        if (errno != EINTR)
        {
            perror("waitpid");
            return 1;
        }
    }
    if (!WIFEXITED(child_status) || WEXITSTATUS(child_status) != 0)
    {
        status = 1; // ошибка дочернего процесса отражается в коде родителя
    }
    return status;
}

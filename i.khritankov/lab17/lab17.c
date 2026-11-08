#define _XOPEN_SOURCE 500 // функции POSIX и работа с широкими символами

#include <stdio.h> // printf, fprintf, perror
#include <stdlib.h> // realloc, free
#include <stdint.h> // SIZE_MAX
#include <string.h> // strcmp, memset, memmove
#include <unistd.h> // read, write, isatty
#include <poll.h> // ожидание продолжения escape-последовательности
#include <termios.h> // tcgetattr, tcsetattr, ERASE и KILL
#include <signal.h> // sigaction, sig_atomic_t
#include <errno.h> // errno, EINTR
#include <locale.h> // setlocale
#include <wchar.h> // mbrtowc, wcrtomb, wcwidth
#include <wctype.h> // iswprint, iswspace
#include <limits.h> // MB_LEN_MAX

static volatile sig_atomic_t stopped = 0; // сигнал завершения из другого процесса

static void stop_handler(int number)
{
    stopped = number; // настройки терминала восстановит основной код
}

static int print_bytes(const char *text, size_t length) // учитываем неполную запись
{
    ssize_t result;

    while (length > 0)
    {
        result = write(STDOUT_FILENO, text, length);
        if (result == -1 && errno == EINTR && !stopped)
        {
            continue;
        }
        if (result <= 0)
        {
            return -1;
        }
        text += result;
        length -= (size_t)result;
    }
    return 0;
}

static int print_character(wchar_t ch)
{
    char bytes[MB_LEN_MAX];
    mbstate_t state;
    size_t length;

    memset(&state, 0, sizeof(state));
    length = wcrtomb(bytes, ch, &state); // кодируем символ для текущей локали
    if (length == (size_t)-1)
    {
        return -1;
    }
    return print_bytes(bytes, length);
}

static int redraw(const wchar_t *line, size_t length) // заново показываем текущую строку
{
    size_t i;

    if (print_bytes("\r", 1) == -1)
    {
        return -1;
    }
    for (i = 0; i < length; i++)
    {
        if (print_character(line[i]) == -1)
        {
            return -1;
        }
    }
    return print_bytes("\033[K", 3); // убираем остаток прежнего текста справа
}

static int columns(const wchar_t *line, size_t length)
{
    size_t i;
    int width = 0;

    for (i = 0; i < length; i++)
    {
        width += wcwidth(line[i]); // русская буква занимает один столбец, а не два байта
    }
    return width;
}

static int edit_line(const struct termios *original, wchar_t **storage)
{
    wchar_t *line = NULL; // текст занимает не более 40 столбцов терминала
    wchar_t *grown;
    size_t length = 0;
    size_t split;
    size_t moved;
    mbstate_t state;
    wchar_t ch;
    unsigned char byte;
    ssize_t result;
    size_t converted;
    int width;
    int erase_key;
    int kill_key;
    int escape = 0; // 1: после ESC, 2: CSI, 3: SS3 или продолжение функциональной клавиши
    struct pollfd input;

    input.fd = STDIN_FILENO;
    input.events = POLLIN;
    memset(&state, 0, sizeof(state));
    while (!stopped)
    {
        if (escape)
        {
            result = poll(&input, 1, 100); // отдельный Escape не должен поглощать следующий обычный ввод
            if (result == -1 && errno == EINTR)
            {
                continue;
            }
            if (result == -1)
            {
                perror("poll");
                return 1;
            }
            if (result == 0)
            {
                escape = 0;
                continue;
            }
        }
        result = read(STDIN_FILENO, &byte, 1); // получаем символ без ожидания Enter
        if (result == -1 && errno == EINTR)
        {
            continue;
        }
        if (result == 0) // терминал закрыл поток ввода
        {
            return 0;
        }
        if (result == -1)
        {
            perror("read");
            return 1;
        }

        if (escape && byte >= 32 && byte != 127)
        {
            if (escape == 1)
            {
                escape = byte == '[' ? 2 : (byte == 'O' ? 3 : 0);
            }
            else if (escape == 2 && byte == '[')
            {
                escape = 3; // вариант функциональных клавиш ESC [[ A
            }
            else if (byte >= '@' && byte <= '~')
            {
                escape = 0; // последний байт стрелки, Home, Delete, F1 и других клавиш
            }
            continue; // служебные байты клавиши не попадают в текст
        }
        escape = 0;
        erase_key = original->c_cc[VERASE] != _POSIX_VDISABLE && byte == original->c_cc[VERASE];
        kill_key = original->c_cc[VKILL] != _POSIX_VDISABLE && byte == original->c_cc[VKILL];
        if (erase_key || kill_key || byte == 23)
        {
            memset(&state, 0, sizeof(state));
            if (erase_key) // ERASE удаляет последний символ
            {
                if (length > 0)
                {
                    length--;
                }
            }
            else if (kill_key) // KILL удаляет текущую строку
            {
                length = 0;
            }
            else // Ctrl-W удаляет последнее слово и пробелы после него
            {
                while (length > 0 && iswspace(line[length - 1]))
                {
                    length--;
                }
                while (length > 0 && !iswspace(line[length - 1]))
                {
                    length--;
                }
            }
            if (redraw(line, length) == -1)
            {
                return 1;
            }
            continue;
        }
        if (byte == 4 && columns(line, length) == 0) // Ctrl-D завершает работу только при курсоре в начале строки
        {
            return 0;
        }
        if (byte < 32 || byte == 127) // остальные управляющие символы дают звуковой сигнал
        {
            memset(&state, 0, sizeof(state));
            escape = byte == 27; // после BEL пропускаем оставшуюся последовательность клавиши
            if (print_bytes("\a", 1) == -1)
            {
                return 1;
            }
            continue;
        }

        converted = mbrtowc(&ch, (const char *)&byte, 1, &state);
        if (converted == (size_t)-2) // ждём оставшиеся байты многобайтного символа
        {
            continue;
        }
        if (converted == (size_t)-1)
        {
            memset(&state, 0, sizeof(state));
            if (print_bytes("\a", 1) == -1)
            {
                return 1;
            }
            continue;
        }
        width = wcwidth(ch);
        if (!iswprint(ch) || width < 0 || width > 40)
        {
            if (print_bytes("\a", 1) == -1)
            {
                return 1;
            }
            continue;
        }

        if (columns(line, length) + width > 40)
        {
            split = length;
            while (split > 0 && !iswspace(line[split - 1]))
            {
                split--; // находим начало слова, которое пересекло границу строки
            }
            if (split > 0 && split < length && !iswspace(ch) &&
                columns(line + split, length - split) + width <= 40) // переносим целиком, только если слово помещается
            {
                moved = length - split;
                if (redraw(line, split) == -1 || print_bytes("\n", 1) == -1)
                {
                    return 1;
                }
                memmove(line, line + split, moved * sizeof(*line));
                length = moved;
                if (redraw(line, length) == -1) // переносим целое слово на новую строку
                {
                    return 1;
                }
            }
            else // полная строка: новое слово или пробел начинаются со следующей
            {
                if (print_bytes("\n", 1) == -1)
                {
                    return 1;
                }
                length = 0;
            }
        }
        if (length == SIZE_MAX / sizeof(*line)) // проверяем размер до сложения и умножения
        {
            fprintf(stderr, "Line is too long\n");
            return 1;
        }
        grown = realloc(line, (length + 1) * sizeof(*line)); // учитываем и символы нулевой ширины
        if (grown == NULL)
        {
            perror("realloc");
            return 1;
        }
        line = grown;
        *storage = line; // основной код освободит строку при любом завершении
        line[length++] = ch;
        if (print_character(ch) == -1) // введённый символ сразу появляется на экране
        {
            return 1;
        }
    }
    return 128 + stopped;
}

int main(int argc, char *argv[])
{
    wchar_t *line = NULL;
    struct termios original;
    struct termios settings;
    struct sigaction action;
    int status;

    if (argc == 2 && (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0))
    {
        printf("Usage: %s\n"
               "Edit text in 40 columns; words wrap automatically.\n"
               "Words longer than 40 columns are split across lines.\n"
               "ERASE: delete character; KILL: clear line; Ctrl-W: delete word.\n"
               "Ctrl-D at line start: exit. Other control and special keys: bell.\n", argv[0]);
        return 0;
    }
    if (argc != 1)
    {
        fprintf(stderr, "Usage: %s [-h|--help]\n", argv[0]);
        return 1;
    }
    if (!isatty(STDIN_FILENO) || !isatty(STDOUT_FILENO))
    {
        fprintf(stderr, "This program needs a terminal for input and output\n");
        return 1;
    }
    if (setlocale(LC_CTYPE, "") == NULL &&
        setlocale(LC_CTYPE, "C.UTF-8") == NULL &&
        setlocale(LC_CTYPE, "en_US.UTF-8") == NULL) // выбираем запасную локаль, если окружение не поддерживается
    {
        fprintf(stderr, "Cannot use the current character locale\n");
        return 1;
    }
    if (tcgetattr(STDIN_FILENO, &original) == -1)
    {
        perror("tcgetattr");
        return 1;
    }
    action.sa_handler = stop_handler;
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0;
    if (sigaction(SIGTERM, &action, NULL) == -1 || sigaction(SIGHUP, &action, NULL) == -1 ||
        sigaction(SIGINT, &action, NULL) == -1 || sigaction(SIGQUIT, &action, NULL) == -1)
    {
        perror("sigaction");
        return 1;
    }

    settings = original;
    settings.c_lflag &= ~(ECHO | ICANON | ISIG | IEXTEN); // управляющие клавиши обрабатывает программа
    settings.c_iflag &= ~(IXON | ICRNL | INLCR | IGNCR | ISTRIP); // не теряем Ctrl-S/Ctrl-Q и старший бит
    settings.c_cc[VMIN] = 1;
    settings.c_cc[VTIME] = 0;
    if (tcsetattr(STDIN_FILENO, TCSANOW, &settings) == -1)
    {
        perror("tcsetattr");
        return 1;
    }
    printf("Edit text: 40 columns. Ctrl-D at line start exits.\n");
    fflush(stdout);
    status = edit_line(&original, &line);
    free(line);
    if (tcsetattr(STDIN_FILENO, TCSANOW, &original) == -1) // возвращаем исходные настройки при выходе
    {
        perror("tcsetattr");
        status = 1;
    }
    return status;
}

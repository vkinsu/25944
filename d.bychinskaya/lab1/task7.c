#define _XOPEN_SOURCE 600

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/mman.h>

struct Line {
    off_t offset;
    off_t length;
};

static volatile sig_atomic_t timed_out = 0;

static void handle_alarm(int sig)
{
    (void)sig;
    timed_out = 1;
}

static int add_line(struct Line **table, size_t *count,
                    off_t offset, off_t length)
{
    struct Line *tmp;

    if (*count >= (size_t)-1 / sizeof(**table)) {
        fprintf(stderr, "Too many lines\n");
        return -1;
    }

    tmp = realloc(*table, (*count + 1) * sizeof(**table));
    if (tmp == NULL) {
        perror("realloc");
        return -1;
    }

    *table = tmp;
    tmp[*count].offset = offset;
    tmp[*count].length = length;
    ++*count;

    return 0;
}

static int build_table(const char *text, off_t size,
                       struct Line **table, size_t *count)
{
    off_t start = 0;
    off_t pos;

    for (pos = 0; pos < size; ++pos) {
        if (text[pos] == '\n') {
            if (add_line(table, count, start, pos - start + 1) < 0)
                return -1;

            start = pos + 1;
        }
    }

    /* Последняя строка может не заканчиваться переводом строки. */
    if (size > start)
        return add_line(table, count, start, size - start);

    return 0;
}

static int print_part(const char *text, off_t offset, off_t length)
{
    if (length > 0) {
        if (fwrite(text + offset, 1, (size_t)length, stdout) != (size_t)length) {
            perror("fwrite");
            return -1;
        }

        if (text[offset + length - 1] != '\n')
            printf("\n");
    }

    return 0;
}

/* 1 — ввод получен, 0 — EOF, -1 — ошибка, -2 — длинный ввод,
   -3 — время на ввод истекло. */
static int get_input(char *buffer, size_t size, int first_input)
{
    int ch;
    int result;

    timed_out = 0;
    if (first_input)
        alarm(5);

    if (fgets(buffer, (int)size, stdin) == NULL) {
        result = ferror(stdin) ? -1 : 0;
    } else if (strchr(buffer, '\n') != NULL || feof(stdin)) {
        result = 1;
    } else {
        while (!timed_out && (ch = getchar()) != '\n' && ch != EOF)
            ;
        result = -2;
    }
    alarm(0);

    return timed_out ? -3 : result;
}

static int parse_number(char *text, long *number)
{
    char *end;

    errno = 0;
    *number = strtol(text, &end, 10);

    if (text == end || errno == ERANGE)
        return -1;

    while (isspace((unsigned char)*end))
        ++end;

    return *end == '\0' ? 0 : -1;
}

int main(int argc, char *argv[])
{
    int fd;
    int status = EXIT_SUCCESS;
    int result;
    int first_input = 1;
    struct Line *table = NULL;
    size_t count = 0;
    char input[128];
    long number;
    struct sigaction action;
    struct stat info;
    char *text = MAP_FAILED;
    off_t size = 0;

    if (argc != 2) {
        fprintf(stderr, "Usage: %s filename\n", argv[0]);
        return EXIT_FAILURE;
    }

    fd = open(argv[1], O_RDONLY);
    if (fd < 0) {
        perror("open");
        return EXIT_FAILURE;
    }

    if (fstat(fd, &info) < 0) {
        perror("fstat");
        status = EXIT_FAILURE;
        goto done;
    }

    if (!S_ISREG(info.st_mode) || info.st_size < 0 ||
        (unsigned long long)info.st_size > (size_t)-1) {
        fprintf(stderr, "Unsupported file\n");
        status = EXIT_FAILURE;
        goto done;
    }

    size = info.st_size;
    if (size > 0) {
        text = mmap(NULL, (size_t)size, PROT_READ, MAP_PRIVATE, fd, 0);
        if (text == MAP_FAILED) {
            perror("mmap");
            status = EXIT_FAILURE;
            goto done;
        }
    }

    if (build_table(text, size, &table, &count) < 0) {
        status = EXIT_FAILURE;
        goto done;
    }

    memset(&action, 0, sizeof(action));
    action.sa_handler = handle_alarm;
    sigemptyset(&action.sa_mask);
    if (sigaction(SIGALRM, &action, NULL) < 0) {
        perror("sigaction");
        status = EXIT_FAILURE;
        goto done;
    }

    for (;;) {
        printf("Line number (0 to exit): ");
        fflush(stdout);

        result = get_input(input, sizeof(input), first_input);
        first_input = 0;

        if (result == -3) {
            printf("\nTime is up. File contents:\n");
            if (print_part(text, 0, size) < 0)
                status = EXIT_FAILURE;
            break;
        }

        if (result == 0)
            break;

        if (result == -1) {
            fprintf(stderr, "Input error\n");
            status = EXIT_FAILURE;
            break;
        }

        if (result == -2 || parse_number(input, &number) < 0) {
            printf("Enter an integer\n");
            continue;
        }

        if (number == 0) {
            printf("File contents:\n");
            if (print_part(text, 0, size) < 0)
                status = EXIT_FAILURE;
            break;
        }

        if (number < 1 || (unsigned long)number > count) {
            printf("No such line\n");
            continue;
        }

        if (print_part(text, table[number - 1].offset,
                       table[number - 1].length) < 0) {
            status = EXIT_FAILURE;
            break;
        }
    }

done:
    alarm(0);
    free(table);

    if (text != MAP_FAILED && munmap(text, (size_t)size) < 0) {
        perror("munmap");
        status = EXIT_FAILURE;
    }

    if (close(fd) < 0) {
        perror("close");
        status = EXIT_FAILURE;
    }

    return status;
}

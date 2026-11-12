#define _XOPEN_SOURCE 600

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>

struct Line {
    off_t offset;
    off_t length;
};

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

static int build_table(int fd, struct Line **table, size_t *count)
{
    char ch;
    ssize_t n;
    off_t start = 0;
    off_t pos;

    while ((n = read(fd, &ch, 1)) > 0) {
        if (ch == '\n') {
            pos = lseek(fd, 0, SEEK_CUR);
            if (pos == (off_t)-1) {
                perror("lseek");
                return -1;
            }

            if (add_line(table, count, start, pos - start) < 0)
                return -1;

            start = pos;
        }
    }

    if (n < 0) {
        perror("read");
        return -1;
    }

    pos = lseek(fd, 0, SEEK_CUR);
    if (pos == (off_t)-1) {
        perror("lseek");
        return -1;
    }

    /* Последняя строка может не заканчиваться переводом строки. */
    if (pos > start)
        return add_line(table, count, start, pos - start);

    return 0;
}

static int print_part(int fd, off_t offset, off_t length)
{
    char buffer[1025];
    size_t wanted;
    ssize_t n;
    int last = '\n';

    if (lseek(fd, offset, SEEK_SET) == (off_t)-1) {
        perror("lseek");
        return -1;
    }

    while (length > 0) {
        wanted = length > 1024 ? 1024 : (size_t)length;
        n = read(fd, buffer, wanted);

        if (n < 0 && errno == EINTR)
            continue;

        if (n <= 0) {
            fprintf(stderr, "Cannot read the indexed text\n");
            return -1;
        }

        buffer[n] = '\0';

        if (printf("%s", buffer) < 0)
            return -1;

        last = (unsigned char)buffer[n - 1];
        length -= n;
    }

    if (last != '\n')
        printf("\n");

    return 0;
}

/* 1 — ввод получен, 0 — EOF, -1 — ошибка, -2 — слишком длинный ввод. */
static int get_input(char *buffer, size_t size)
{
    int ch;

    if (fgets(buffer, (int)size, stdin) == NULL)
        return ferror(stdin) ? -1 : 0;

    if (strchr(buffer, '\n') != NULL || feof(stdin))
        return 1;

    while ((ch = getchar()) != '\n' && ch != EOF)
        ;

    return -2;
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
    struct Line *table = NULL;
    size_t count = 0;
    char input[128];
    long number;

    if (argc != 2) {
        fprintf(stderr, "Usage: %s filename\n", argv[0]);
        return EXIT_FAILURE;
    }

    fd = open(argv[1], O_RDONLY);
    if (fd < 0) {
        perror("open");
        return EXIT_FAILURE;
    }

    if (build_table(fd, &table, &count) < 0) {
        status = EXIT_FAILURE;
        goto done;
    }

    for (;;) {
        printf("Line number (0 to exit): ");
        fflush(stdout);

        result = get_input(input, sizeof(input));

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

        if (number == 0)
            break;

        if (number < 1 || (unsigned long)number > count) {
            printf("No such line\n");
            continue;
        }

        if (print_part(fd, table[number - 1].offset,
                       table[number - 1].length) < 0) {
            status = EXIT_FAILURE;
            break;
        }
    }

done:
    free(table);

    if (close(fd) < 0) {
        perror("close");
        status = EXIT_FAILURE;
    }

    return status;
}

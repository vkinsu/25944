#define _XOPEN_SOURCE 500

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>
#include <string.h>

struct Line {
    off_t offset;
    size_t length;
};

static volatile sig_atomic_t timed_out = 0;

static void timeout_handler(int sig)
{
    (void)sig;
    timed_out = 1;
}

static int make_table(int fd, struct Line **table, size_t *count)
{
    char ch;
    ssize_t n;
    off_t offset;
    size_t length;
    struct Line *next;

    while (1) {
        offset = lseek(fd, 0, SEEK_CUR);
        if (offset == (off_t)-1) {
            perror("lseek");
            return -1;
        }

        length = 0;

        while (1) {
            n = read(fd, &ch, 1);
            if (n == -1 && errno == EINTR)
                continue;
            if (n == -1) {
                perror("read");
                return -1;
            }
            if (n == 0)
                break;

            length++;
            if (ch == '\n')
                break;
        }

        if (length == 0)
            break;

        next = realloc(*table, (*count + 1) * sizeof(**table));
        if (next == NULL) {
            perror("realloc");
            return -1;
        }

        *table = next;
        (*table)[*count].offset = offset;
        (*table)[*count].length = length;
        (*count)++;

        if (n == 0)
            break;
    }

    return 0;
}

static int print_text(int fd, off_t offset, size_t length)
{
    char buffer[1024];
    ssize_t n;
    size_t portion;

    if (lseek(fd, offset, SEEK_SET) == (off_t)-1) {
        perror("lseek");
        return -1;
    }

    while (length > 0) {
        portion = length < sizeof(buffer) ? length : sizeof(buffer);
        n = read(fd, buffer, portion);

        if (n == -1 && errno == EINTR)
            continue;
        if (n <= 0) {
            perror("read");
            return -1;
        }

        fwrite(buffer, 1, (size_t)n, stdout);
        length -= (size_t)n;
    }

    return 0;
}

static int process_file(int fd)
{
    struct Line *table = NULL;
    size_t count = 0;
    char input[64];
    char *end;
    char *result;
    long number;
    int first_input = 1;
    int status = 0;

    if (make_table(fd, &table, &count) == -1) {
        free(table);
        return 1;
    }

    while (1) {
        printf("Line number (0 to exit): ");
        fflush(stdout);

        if (first_input) {
            timed_out = 0;
            alarm(5);
        }

        result = fgets(input, sizeof(input), stdin);

        if (first_input) {
            alarm(0);
            first_input = 0;
        }

        if (timed_out) {
            printf("\n");
            if (count > 0 &&
                print_text(fd, 0,
                    (size_t)(table[count - 1].offset +
                             table[count - 1].length)) == -1)
                status = 1;
            break;
        }

        if (result == NULL) {
            if (ferror(stdin)) {
                perror("fgets");
                status = 1;
            }
            break;
        }

        if (strchr(input, '\n') == NULL && !feof(stdin)) {
            int ch;
            do {
                ch = getchar();
            } while (ch != '\n' && ch != EOF);
            fprintf(stderr, "Invalid line number\n");
            continue;
        }

        errno = 0;
        number = strtol(input, &end, 10);

        if (errno != 0 || end == input ||
            (*end != '\n' && *end != '\0') ||
            number < 0 || (unsigned long)number > count) {
            fprintf(stderr, "Invalid line number\n");
            continue;
        }

        if (number == 0)
            break;

        if (print_text(fd, table[number - 1].offset,
                       table[number - 1].length) == -1) {
            status = 1;
            break;
        }
    }

    free(table);
    return status;
}

int main(int argc, char *argv[])
{
    int fd;
    int status;
    struct sigaction action;

    if (argc != 2) {
        fprintf(stderr, "Usage: %s file\n", argv[0]);
        return 1;
    }

    action.sa_handler = timeout_handler;
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0;

    if (sigaction(SIGALRM, &action, NULL) == -1) {
        perror("sigaction");
        return 1;
    }

    fd = open(argv[1], O_RDONLY);
    if (fd == -1) {
        perror("open");
        return 1;
    }

    status = process_file(fd);

    if (close(fd) == -1) {
        perror("close");
        status = 1;
    }

    return status;
}

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

#define MAX_LINES 100

int main()
{
    int fd;
    char ch;
    int line_count = 0;
    long offsets[MAX_LINES];
    long lengths[MAX_LINES];
    long start = 0;
    long pos = 0;
    int number;
    long len;
    char *buffer;

    fd = open("text.txt", O_RDONLY);

    if (fd == -1) {
        perror("open");
        return 1;
    }

    offsets[0] = 0;

    while (read(fd, &ch, 1) == 1) {

        pos++;

        if (ch == '\n') {

            lengths[line_count] = pos - start;

            line_count++;

            if (line_count >= MAX_LINES) {
                break;
            }

            offsets[line_count] = pos;
            start = pos;
        }
    }

    if (pos > start && line_count < MAX_LINES) {
        lengths[line_count] = pos - start;
        line_count++;
    }

    printf("Line table:\n");

    for (number = 0; number < line_count; number++) {
        printf("Line %d: offset = %ld, length = %ld\n",
               number + 1,
               offsets[number],
               lengths[number]);
    }

    while (1) {

        printf("\nEnter line number (0 to exit): ");

        if (scanf("%d", &number) != 1) {
            printf("Invalid input\n");
            break;
        }

        if (number == 0) {
            break;
        }

        if (number < 1 || number > line_count) {
            printf("No such line\n");
            continue;
        }

        len = lengths[number - 1];

        buffer = malloc(len + 1);

        if (buffer == NULL) {
            perror("malloc");
            close(fd);
            return 1;
        }

        if (lseek(fd, offsets[number - 1], SEEK_SET) == -1) {
            perror("lseek");
            free(buffer);
            close(fd);
            return 1;
        }

        if (read(fd, buffer, len) != len) {
            perror("read");
            free(buffer);
            close(fd);
            return 1;
        }

        buffer[len] = '\0';

        printf("%s", buffer);

        free(buffer);
    }

    close(fd);

    return 0;
}


#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <sys/wait.h>

int main() {
    int fd[2];
    char text[1000];

    if (pipe(fd) == -1) {
        perror("pipe");
        return 1;
    }

    pid_t pid = fork();

    if (pid == -1) {
        perror("fork");
        close(fd[0]);
        close(fd[1]);
        return 1;
    }

    if (pid == 0) {
        close(fd[0]);

        printf("Введите текст: ");
        fflush(stdout);

        if (fgets(text, sizeof(text), stdin) == NULL) {
            fprintf(stderr, "Ошибка ввода текста\n");
            close(fd[1]);
            return 1;
        }

        size_t len = strlen(text);
        size_t sent = 0;

        while (sent < len) {
            ssize_t n = write(fd[1], text + sent, len - sent);

            if (n <= 0) {
                perror("write");
                close(fd[1]);
                return 1;
            }

            sent += n;
        }

        close(fd[1]);
        return 0;
    }
    else {
        close(fd[1]);

        char c;
        ssize_t n;

        printf("Результат: ");
        fflush(stdout);

        while ((n = read(fd[0], &c, 1)) > 0) {
            putchar(toupper((unsigned char)c));
        }

        if (n == -1)
            perror("read");

        close(fd[0]);
        waitpid(pid, NULL, 0);
    }

    return 0;
}

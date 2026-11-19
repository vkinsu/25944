
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <termios.h>
#include <errno.h>

#define MAX_LEN 4095

static struct termios old;
static int changed = 0;

void restore(void) {
    if (changed)
        tcsetattr(0, TCSANOW, &old);
}

void bell(void) {
    putchar('\a');
    fflush(stdout);
}

void redraw(char *line, int len, int *rows) {
    printf("\r");

    if (*rows > 0)
        printf("\033[%dA", *rows);

    printf("\033[J");

    int col = 0;
    *rows = 0;

    for (int i = 0; i < len;) {
        if (line[i] == ' ') {
            if (col == 40) {
                putchar('\n');
                (*rows)++;
                col = 0;
            }

            putchar(' ');
            col++;
            i++;
        }
        else {
            int word = 0;

            while (i + word < len &&
                   line[i + word] != ' ')
                word++;

            if (col > 0 && col + word > 40) {
                putchar('\n');
                (*rows)++;
                col = 0;
            }

            while (word > 0) {
                if (col == 40) {
                    putchar('\n');
                    (*rows)++;
                    col = 0;
                }

                putchar(line[i++]);
                col++;
                word--;
            }
        }
    }

    fflush(stdout);
}

int main() {
    struct termios t;
    char line[MAX_LEN + 1] = "";
    int len = 0, rows = 0;
    unsigned char c;

    if (!isatty(0) || !isatty(1)) {
        fprintf(stderr, "Запустите в терминале!\n");
        return 1;
    }

    if (tcgetattr(0, &old) == -1) {
        perror("tcgetattr");
        return 1;
    }

    atexit(restore);

    t = old;
    t.c_lflag &= ~(ECHO | ICANON | ISIG | IEXTEN);
    t.c_cc[VMIN] = 1;
    t.c_cc[VTIME] = 0;

    if (tcsetattr(0, TCSANOW, &t) == -1) {
        perror("tcsetattr");
        return 1;
    }

    changed = 1;

    printf("Редактор: Ctrl-D на пустой строке - выход\n");
    fflush(stdout);

    while (1) {
        ssize_t n = read(0, &c, 1);

        if (n == -1) {
            if (errno == EINTR)
                continue;

            perror("read");
            return 1;
        }

        if (n == 0)
            break;

        if (c == 4 && len == 0)
            break;

        if (c == (unsigned char)old.c_cc[VERASE]) {
            if (len == 0)
                bell();
            else
                line[--len] = '\0';
        }
        else if (c == (unsigned char)old.c_cc[VKILL]) {
            if (len == 0)
                bell();
            else {
                len = 0;
                line[0] = '\0';
            }
        }
        else if (c == 23) {
            if (len == 0) {
                bell();
            }
            else {
                while (len > 0 && line[len - 1] == ' ')
                    len--;

                while (len > 0 && line[len - 1] != ' ')
                    len--;

                line[len] = '\0';
            }
        }
        else if (c == '\n' || c == '\r') {
            putchar('\n');
            fflush(stdout);

            len = 0;
            rows = 0;
            line[0] = '\0';
            continue;
        }
        else if (c >= 32 && c <= 126) {
            if (len == MAX_LEN) {
                bell();
                continue;
            }

            line[len++] = c;
            line[len] = '\0';
        }
        else {
            bell();
            continue;
        }

        redraw(line, len, &rows);
    }

    printf("\nРедактор завершён\n");
    return 0;
}

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <sys/termios.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>

#define BELL do { \
    write(1, "\a", 1); \
    system("paplay /usr/share/sounds/freedesktop/stereo/bell.oga </dev/null >/dev/null 2>&1 &"); \
} while (0)
#define BACKSPASE write(1, "\b \b", 3)
#define MAX_LINE_LENGHT 40

const char *red = "\x1b[31m";
const char *reset = "\x1b[0m";
const char *green = "\x1b[32m";
const char *purple = "\x1b[35m";

void wrap_line(char *line, int *pos) {
    int end = *pos, start = *pos;

    while (start > 0 && line[start-1] != ' ') start--;

    if (start == 0 || start == end) {
        write(1, "\n", 1);
        *pos = 0;
        return;
    }

    for (int i = start; i < end; i++) BACKSPASE;
    write(1, "\n", 1);
    write(1, line + start, end - start);
    memmove(line, line + start, end - start);
    *pos = end - start;
}

int main() {
    struct termios tty_orig, tty;
    char line [MAX_LINE_LENGHT + 1];
    int pos = 0;
    unsigned char tempChar;
    unsigned char eofChar, eraseChar, killChar, werasChar;

    if (isatty(0)!=1) {
        printf("%sERROR : stdin does not represent a terminal\n%s", red, reset);
        exit(1);
    }

    if (tcgetattr(0,&tty) == -1) {
        printf("%sERROR : failed to get terminal settings\n%s", red, reset);
        exit(2);
    }

    tty_orig = tty;

    eofChar = tty_orig.c_cc[VEOF];
    eraseChar = tty_orig.c_cc[VERASE];
    killChar = tty_orig.c_cc[VKILL];
    werasChar = tty_orig.c_cc[VWERASE];

    tty.c_lflag &= ~(ICANON | ISIG | ECHO);
    tty.c_cc[VMIN] = 1;
    tty.c_cc[VTIME] = 0;

    if(tcsetattr(0, TCSANOW, &tty)==-1) {
        printf("%sERROR : failed to set terminal settings\n%s", red, reset);
        exit(3);
    }

    while(read(0,&tempChar,1)>0) {

        if (tempChar == eofChar && pos == 0) break;

        else if(tempChar == eraseChar && pos>0) {
            BACKSPASE;
            pos--;
        }

        else if(tempChar == killChar && pos>0) {
            while(pos>0) {
                BACKSPASE;
                pos--;
            }
        }

        else if(tempChar == werasChar && pos>0) {
            while(pos>0 && line[pos-1]==32) {
                BACKSPASE;
                pos--;
            }
            while(pos>0 && line[pos-1]!=32) {
                BACKSPASE;
                pos--;
            }
        }

        else if(tempChar == '\n') {
            pos=0;
            write(1,&tempChar,1);
        }

        else if(!isprint(tempChar)) BELL;

        else {
            if (pos == MAX_LINE_LENGHT) {
                if (tempChar == ' ') {
                    write(1, "\n", 1);
                    pos = 0;
                    continue;
                }
                wrap_line(line, &pos);
            }
            write(1,&tempChar,1);
            line[pos++] = tempChar;
        }
    }

    tcsetattr(0,TCSANOW,&tty_orig);
    return 0;
}

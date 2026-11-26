#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <termios.h>
#include <ctype.h>
#include <errno.h>
#include <poll.h>

// #define LIMIT 4096
#define WIDTH 40
#define ESC_TIMEOUT_MS 150

void eraseLastChar(char *input_str, size_t *length){
    if(*length == 0)
        return;
    
    (*length)--;
    input_str[*length] = '\0';
    
}

void killLine(char *input_str, size_t *length){
    while(*length > 0)
        eraseLastChar(input_str, length);

}

void eraseLastWord(char *input_str, size_t *length){
    while(*length > 0 && input_str[*length - 1] == ' ')
        eraseLastChar(input_str, length);

    while(*length > 0 && input_str[*length - 1] != ' ')
        eraseLastChar(input_str, length);
}

int writeAll(const char *text, size_t length){
    ssize_t n = 0;
    while(length > 0){
        n = write(STDOUT_FILENO, text, length);

        if(n == -1){
            if(errno == EINTR)
                continue;
            return -1;
        }
        if(n == 0){
            errno = EIO;
            return -1;
        }

        text += n;
        length -= (size_t)n;
    }

    return 0;
}

int redrawLine(const char *text, size_t length){
    if(writeAll("\r\033[2K", 5) == -1)
        return -1;
    
    return writeAll(text, length);
}

int appendChar(char *text, size_t *length, char ch){
    if(*length == WIDTH){
        if(ch == ' '){
            if(writeAll("\r\n", 2) == -1)
                return -1;

            *length = 0;
            text[0] = '\0';
            return 0;
        }

        size_t word_start = *length;

        while(word_start > 0 && text[word_start-1] != ' ')
            word_start--;

        size_t word_length = *length - word_start;

        if(word_start > 0 && word_length > 0){
            if(redrawLine(text, word_start) == -1)
                return -1;

            if(writeAll("\r\n", 2) == -1)
                return -1;

            memmove(text, text + word_start, word_length);
            *length = word_length;
        }
        else{
            if(writeAll("\r\n", 2) == -1)
                return -1;

            *length = 0;
        }

        text[*length] = '\0';
    }

    text[*length] = ch;
    (*length)++;
    text[*length] = '\0';

    return redrawLine(text, *length);
}


ssize_t readTerminalByte(char *ch, int timeout_ms){
    struct pollfd input = {STDIN_FILENO, POLLIN, 0};
    int ready = poll(&input, 1, timeout_ms);

    if(ready == -1)
        return -1;
    if(ready == 0)
        return -2;
    if(input.revents & POLLNVAL){
        errno = EBADF;
        return -1;
    }
    if(input.revents & POLLERR){
        errno = EIO;
        return -1;
    }

    return read(STDIN_FILENO, ch, 1);
}


int skipEscapeByte(unsigned char ch, int *state){
    if(*state == 1){
        *state = 0;
        if(ch == '[' || ch == 'O'){
            *state = 2;
            return 1;
        }
    }
    else if(*state == 2){
        if(ch >= 0x40 && ch <= 0x7e){
            *state = 0;
            return 1;
        }
        if(ch >= 0x20 && ch <= 0x3f)
            return 1;

        *state = 0;
    }

    return 0;
}

int main(void){

    char input_str[WIDTH + 1];
    input_str[0] = '\0';
    size_t length = 0;
    int result = 0;

    struct termios original;
    struct termios modified;

    if(tcgetattr(STDIN_FILENO, &original) == -1){
        perror("tcgetattr");
        exit(1);
    }

    modified = original;

    modified.c_lflag &= ~(ICANON | ECHO | ISIG | IEXTEN);
    modified.c_iflag &= ~IXON;

    modified.c_cc[VMIN] = 1;
    modified.c_cc[VTIME] = 0;

    if(tcsetattr(STDIN_FILENO, TCSAFLUSH, &modified) == -1){
        perror("tcsetattr");
        exit(1);
    }

    char ch;
    ssize_t n;

    int escape_state = 0;

    while(1){
        n = readTerminalByte(&ch, escape_state ? ESC_TIMEOUT_MS : -1);
        if(n == -2){
            escape_state = 0;
            continue;
        }
        if(n == -1 && errno == EINTR)
            continue;
        if(n != 1)
            break;

        if(skipEscapeByte((unsigned char)ch, &escape_state))
            continue;

        if((unsigned char)ch == 0x1b){
            if(writeAll("\a", 1) == -1){
                perror("write");
                result = 1;
                break;
            }
            escape_state = 1;
            continue;
        }

        if(ch == 4){
            if(length == 0)
                break;
            
            if(writeAll("\a", 1) == -1){
                perror("write");
                result = 1;
                break;
            }
            continue;
        }
        if((unsigned char)ch == original.c_cc[VERASE]){
            eraseLastChar(input_str, &length);

            if(redrawLine(input_str, length) == -1){
                perror("redrawLine");
                result = 1;
                break;
            }
            continue;
        }
        if((unsigned char)ch == original.c_cc[VKILL]){
            killLine(input_str, &length);

            if(redrawLine(input_str, length) == -1){
                perror("redrawLine");
                result = 1;
                break;
            }
            continue;
        }
        if((unsigned char)ch == 0x17){
            eraseLastWord(input_str, &length);

            if(redrawLine(input_str, length) == -1){
                perror("redrawLine");
                result = 1;
                break;
            }
            continue;
        }
        if(isprint((unsigned char)ch)){
            if(appendChar(input_str, &length, ch) == -1){
                perror("appendChar");
                result = 1;
                break;
            }
        }
        else{
            if(writeAll("\a", 1) == -1){
                perror("write");
                result = 1;
                break;
            }
        }
    }

    if(n == -1){
        perror("read");
        result = 1;
    }

    if(tcsetattr(STDIN_FILENO, TCSANOW, &original) == -1){
        perror("tcsetattr restore");
        exit(1);
    }

    exit(result);
}
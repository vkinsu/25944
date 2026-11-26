#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <ctype.h>
#include <errno.h>

int writeAll(int fd, const char *text, size_t length){
    ssize_t n = 0;
    while(length > 0){
        n = write(fd, text, length);

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

int main(void){
    int channel[2];

    if(pipe(channel) == -1){
        perror("pipe");
        exit(1);
    }

    pid_t pid = fork();

    if(pid == -1){
        perror("fork");
        close(channel[0]);
        close(channel[1]);
        exit(1);
    }

    if(pid == 0){
        close(channel[1]);

        char buffer[256];

        ssize_t n = 0;

        while(1){
            n = read(channel[0], buffer, sizeof(buffer));

            if(n == -1){
                if(errno == EINTR)
                    continue;

                perror("read");
                close(channel[0]);
                _exit(1);
            }

            if(n == 0)
                break;

            for(ssize_t i=0; i < n; i++)
                buffer[i] = (char)toupper((unsigned char)buffer[i]);

            if(writeAll(STDOUT_FILENO, buffer, (size_t)n) == -1){
                perror("write stdout");
                close(channel[0]);
                _exit(1);
            }
        }

        close(channel[0]);
        _exit(0);
    }
    else{
        close(channel[0]);

        const char text[] = "Hello, World!\nPipe test: AbCdEf 12345\n";
        int result = 0;

        if(writeAll(channel[1], text, sizeof(text)-1) == -1){
            perror("write pipe");
            result = 1;
        }

        close(channel[1]);

        if(waitpid(pid, NULL, 0) == -1){
            perror("waitpid");
            exit(1);
        }

        exit(result);
    }

    exit(0);
}
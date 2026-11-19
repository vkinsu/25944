#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <wait.h>

int main(){
    int pipefd[2];
    if (pipe(pipefd)==-1){
        perror("error in pipe");
        exit(1);
    }
    pid_t pid;
    char str;
    char* line=(char*)malloc(sizeof(char)*1000);
    switch(pid=fork()){
        case -1:
            perror("error in fork");
            exit(1);
        case 0:
            close(pipefd[0]);
            fgets(line, 1000, stdin);
            write(pipefd[1], line, strlen(line));
            str='\0';
            write(pipefd[1], &str, 1);
            close(pipefd[1]);
        default:
            wait(NULL);
            close(pipefd[1]);
            while (1){
                read(pipefd[0], &str, 1);
                str=(char)toupper((int)str);
                if (str=='\0'){
                    break;
                }
                printf("%c", str);
            }
            close(pipefd[0]);
            exit(0);
    }
    free(line);
    exit(0);
}

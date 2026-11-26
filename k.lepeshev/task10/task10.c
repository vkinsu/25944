#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>


int main(int argc, char *argv[]){

    if(argc < 2){
        printf("Usage: %s command [arguments...]\n", argv[0]);
        exit(1);
    }

    printf("Parent process: creating child\n");
    fflush(stdout);

    pid_t pid = fork();

    if(pid == -1){
        perror("fork");
        exit(1);
    }
    else if(pid == 0){
        execvp(argv[1], &argv[1]);

        perror("execvp");
        _exit(127);
    }
    else{
        int status = 0;
        if(waitpid(pid, &status, 0) == -1){
            perror("waitpid");
            exit(1);
        }
        printf("Parent process: child has finished\n");

        if(WIFEXITED(status)){
            printf("Exit code: %d\n", WEXITSTATUS(status));
        }
        else if(WIFSIGNALED(status)){
            printf("Terminated by signal: %d\n", WTERMSIG(status));
        }
    }

    exit(0);
}
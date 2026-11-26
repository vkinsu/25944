#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>

volatile sig_atomic_t beep_count = 0;
volatile sig_atomic_t finished = 0;

void handleInt(int signo){
    (void)signo;

    int saved_errno = errno;

    if(write(STDOUT_FILENO, "\a", 1) == 1)
        beep_count++;

    errno = saved_errno;
}

void handleQuit(int signo){
    (void)signo;
    finished = 1;
}

int main(void){
    struct sigaction action = {0};

    sigset_t blocked;
    sigset_t original_mask;

    sigemptyset(&blocked);
    sigaddset(&blocked, SIGINT);
    sigaddset(&blocked, SIGQUIT);

    if(sigprocmask(SIG_BLOCK, &blocked, &original_mask) == -1){
        perror("sigprocmask");
        exit(1);
    }

    sigemptyset(&action.sa_mask);
    sigaddset(&action.sa_mask, SIGINT);
    sigaddset(&action.sa_mask, SIGQUIT);

    action.sa_handler = handleInt;

    if(sigaction(SIGINT, &action, NULL) == -1){
        perror("sigaction SIGINT");
        exit(1);
    }

    action.sa_handler = handleQuit;

    if(sigaction(SIGQUIT, &action, NULL) == -1){
        perror("sigaction SIGQUIT");
        exit(1);
    }

    sigset_t wait_mask = original_mask;

    sigdelset(&wait_mask, SIGINT);
    sigdelset(&wait_mask, SIGQUIT);

    while(!finished){
        if(sigsuspend(&wait_mask) == -1 && errno != EINTR){
            perror("sigsuspend");
            exit(1);
        }
    }

    printf("\nBeep count: %d\n", (int)beep_count);

    exit(0);
}

#include <stdio.h>
#include <unistd.h>
#include <signal.h>

volatile sig_atomic_t count = 0;
volatile sig_atomic_t stop = 0;

void handle_int(int sig) {
    count++;
    write(STDOUT_FILENO, "\a", 1);
}

void handle_quit(int sig) {
    stop = 1;
}

int main() {
    struct sigaction sa = {0};

    sa.sa_handler = handle_int;
    sigemptyset(&sa.sa_mask);

    if (sigaction(SIGINT, &sa, NULL) == -1) {
        perror("sigaction");
        return 1;
    }

    sa.sa_handler = handle_quit;

    if (sigaction(SIGQUIT, &sa, NULL) == -1) {
        perror("sigaction");
        return 1;
    }

    printf("Ctrl-C - пищать\n");
    printf("Ctrl-\\ - завершить\n");
    fflush(stdout);

    while (!stop)
        pause();

    printf("\nКоличество звуковых сигналов: %d\n", (int)count);

    return 0;
}

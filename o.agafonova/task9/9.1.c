
#include <stdio.h>
#include <unistd.h>

int main() {
    pid_t pid = fork();

    if (pid == 0) {
        execlp("cat", "cat", "text.txt", NULL);
        perror("execlp");
        return 1;
    }
    else if (pid > 0) {
        printf("Родитель работает\n");
        printf("Родитель завершает работу\n");
    }
    else {
        perror("fork");
        return 1;
    }

    return 0;
}

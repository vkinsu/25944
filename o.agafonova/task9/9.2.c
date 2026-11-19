
#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>

int main() {
    pid_t pid = fork();

    if (pid == 0) {
        execlp("cat", "cat", "text.txt", NULL);
        perror("execlp");
        return 1;
    }
    else if (pid > 0) {
        printf("Родитель работает\n");

        wait(NULL);

        printf("\nДочерний процесс завершился\n");
    }
    else {
        perror("fork");
        return 1;
    }

    return 0;
}

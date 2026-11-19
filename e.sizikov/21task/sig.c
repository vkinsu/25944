#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>

int k = 0;

void mysignal(int var)
{
    k += 1;
    write(1, "\a", 1);
    signal(SIGINT, mysignal);
}

void mysignal2(int var)
{
    printf("\n%d\n", k);
    exit(0);
}

int main()
{
    signal(SIGINT, mysignal);
    signal(SIGQUIT, mysignal2);
    while (1)
    {
        pause();
    }
    exit(0);
}

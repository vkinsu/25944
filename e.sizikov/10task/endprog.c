#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <errno.h>
int main(int argc, char* argv[])
{   
    if (argc>1)
    {
        pid_t pid;
        switch(pid=fork())
        {
            int status;
            case -1:
                perror("error in fork");
                exit(1);
            case 0:
                if (execvp(argv[1], &argv[1])==-1)
		{
		    perror("error in execvp");
		}
            default:
                waitpid(pid, &status, 0);
		printf("%d\n", status);
        }
    }
    else
    {
        perror("using new program");
    }
    exit(0);
}

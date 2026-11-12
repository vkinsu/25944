#include <stdio.h>
#include <unistd.h>

static int check_file(void)
{
    FILE *file;

    printf("Real UID: %ld\n", (long)getuid());
    printf("Effective UID: %ld\n", (long)geteuid());
    fflush(stdout);

    file = fopen("data.txt", "r");

    if (file == NULL)
    {
        perror("fopen");
        return 1;
    }

    printf("File opened successfully\n");

    if (fclose(file) == EOF)
    {
        perror("fclose");
        return 1;
    }

    return 0;
}

int main(void)
{
    int status;

    status = check_file();

    if (setuid(getuid()) == -1)
    {
        perror("setuid");
        return 1;
    }

    if (check_file() != 0)
    {
        status = 1;
    }

    return status;
}

#define _XOPEN_SOURCE 500

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/resource.h>
#include <errno.h>
#include <limits.h>

extern char **environ;
static long nproc_override = -1;

static int get_number(const char *str, long *value)
{
    char *end;

    errno = 0;
    *value = strtol(str, &end, 10);

    if (errno != 0 || end == str ||
        *end != '\0' || *value < 0)
    {
        return -1;
    }

    return 0;
}

static int process_options(int argc, char *argv[])
{
    int option;
    int bad_option;
    int status;
    char *argument;

    long value;
    struct rlimit limit;
    char path[PATH_MAX];
    char **env;

    option = getopt(argc, argv, ":ispuU:cC:dvV:");

    if (option == -1)
    {
        return 0;
    }

    argument = optarg;
    bad_option = optopt;

    status = process_options(argc, argv);

    switch (option)
    {
        case 'i':
            printf("Real UID: %ld\n", (long)getuid());
            printf("Effective UID: %ld\n", (long)geteuid());
            printf("Real GID: %ld\n", (long)getgid());
            printf("Effective GID: %ld\n", (long)getegid());
            break;

        case 's':
            if (setpgid(0, 0) == -1)
            {
                perror("setpgid");
                status = 1;
            }
            break;

        case 'p':
            printf("PID: %ld\n", (long)getpid());
            printf("PPID: %ld\n", (long)getppid());
            printf("PGID: %ld\n", (long)getpgrp());
            break;

        case 'u':
            errno = 0;
            value = nproc_override;

            if (value == -1)
            {
                value = sysconf(_SC_CHILD_MAX);
            }

            if (value == -1 && errno != 0)
            {
                perror("sysconf");
                status = 1;
            }
            else if (value == -1)
            {
                printf("unlimited\n");
            }
            else
            {
                printf("%ld\n", value);
            }
            break;

        case 'U':
            if (get_number(argument, &value) == -1)
            {
                fprintf(stderr,
                        "Invalid ulimit value: %s\n",
                        argument);
                status = 1;
            }
            else
            {
                nproc_override = value;
                printf("%ld\n", value);
            }
            break;

        case 'c':
            if (getrlimit(RLIMIT_CORE, &limit) == -1)
            {
                perror("getrlimit");
                status = 1;
            }
            else if (limit.rlim_cur == RLIM_INFINITY)
            {
                printf("unlimited\n");
            }
            else
            {
                printf("%llu\n",
                       (unsigned long long)limit.rlim_cur);
            }
            break;

        case 'C':
            if (get_number(argument, &value) == -1)
            {
                fprintf(stderr,
                        "Invalid core size: %s\n",
                        argument);
                status = 1;
            }
            else if (getrlimit(RLIMIT_CORE, &limit) == -1)
            {
                perror("getrlimit");
                status = 1;
            }
            else
            {
                limit.rlim_cur = (rlim_t)value;

                if (setrlimit(RLIMIT_CORE, &limit) == -1)
                {
                    perror("setrlimit");
                    status = 1;
                }
            }
            break;

        case 'd':
            if (getcwd(path, sizeof(path)) == NULL)
            {
                perror("getcwd");
                status = 1;
            }
            else
            {
                printf("%s\n", path);
            }
            break;

        case 'v':
            for (env = environ; *env != NULL; env++)
            {
                printf("%s\n", *env);
            }
            break;

        case 'V':
            if (putenv(argument) == -1)
            {
                perror("putenv");
                status = 1;
            }
            break;

        case '?':
            fprintf(stderr,
                    "Unknown option: -%c\n",
                    bad_option);
            status = 1;
            break;

        case ':':
            fprintf(stderr,
                    "Option -%c requires an argument\n",
                    bad_option);
            status = 1;
            break;
    }

    return status;
}

static void normalize_options(int argc, char *argv[])
{
    int i;

    for (i = 1; i < argc; i++)
    {
        char *src = argv[i];
        char *dst = argv[i];
        int valid = 1;
        int j;

        if (src[0] != '-' || src[1] == '\0')
            continue;

        for (j = 0; src[j] != '\0'; j += 2)
        {
            char ch;

            if (src[j] != '-' || src[j + 1] == '\0')
            {
                valid = 0;
                break;
            }

            ch = src[j + 1];

            if (ch != 'i' && ch != 's' &&
                ch != 'p' && ch != 'u' &&
                ch != 'c' && ch != 'd' &&
                ch != 'v')
            {
                valid = 0;
                break;
            }
        }

        if (!valid || j <= 2)
            continue;

        *dst++ = '-';

        for (j = 1; src[j] != '\0'; j++)
        {
            if (src[j] != '-')
                *dst++ = src[j];
        }

        *dst = '\0';
    }
}

int main(int argc, char *argv[])
{
    if (argc == 1)
    {
        printf("Usage: %s [options]\n"
               "Options are executed from right to left.\n"
               "  -i            Print real and effective UID/GID\n"
               "  -s            Become a process group leader\n"
               "  -p            Print PID, PPID and PGID\n"
               "  -u            Print user process limit or local value\n"
               "  -Uvalue       Set and print local value for -u (this run only)\n"
               "  -c            Print core file limit in bytes\n"
               "  -Csize        Set core file limit in bytes\n"
               "  -d            Print current directory\n"
               "  -v            Print environment variables\n"
               "  -Vname=value  Set an environment variable\n",
               argv[0]);
        return 0;
    }

    normalize_options(argc, argv);
    opterr = 0;

    return process_options(argc, argv);
}

#define _XOPEN_SOURCE 500

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int main(void)
{
    time_t now;
    struct tm *sp;

    char *zones[] =
    {
        "TZ=America/Los_Angeles",
        "TZ=PST8",
        "TZ=PDT7"
    };

    const char *labels[] =
    {
        "California",
        "PST",
        "PDT"
    };

    int i;

    if (time(&now) == (time_t)-1)
    {
        perror("time");
        return 1;
    }

    for (i = 0; i < 3; i++)
    {
        if (putenv(zones[i]) != 0)
        {
            perror("putenv");
            return 1;
        }

        sp = localtime(&now);

        if (sp == NULL)
        {
            perror("localtime");
            return 1;
        }

        printf("%s: %02d/%02d/%04d %02d:%02d:%02d %s\n",
               labels[i],
               sp->tm_mon + 1,
               sp->tm_mday,
               sp->tm_year + 1900,
               sp->tm_hour,
               sp->tm_min,
               sp->tm_sec,
               tzname[sp->tm_isdst > 0]);
    }

    return 0;
}


#define _XOPEN_SOURCE 500

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

static void print_time(const char *name, const char *zone,
                       time_t now)
{
    struct tm *sp;
    char buf[100];
    static char tzbuf[100];

    snprintf(tzbuf, sizeof(tzbuf), "TZ=%s", zone);
    putenv(tzbuf);
    tzset();

    sp = localtime(&now);

    if (sp == NULL)
    {
        printf("Error getting time\n");
        return;
    }

    strftime(buf, sizeof(buf),
             "%m/%d/%Y %H:%M:%S", sp);

    printf("%s: %s %s\n", name, buf,
           sp->tm_isdst > 0 ? "PDT" : "PST");
}

int main(void)
{
    time_t now = time(NULL);

    print_time("California",
               "PST8PDT,M3.2.0/2,M11.1.0/2", now);

    print_time("PST", "PST8", now);

    print_time("PDT", "PDT7", now);

    return 0;
}



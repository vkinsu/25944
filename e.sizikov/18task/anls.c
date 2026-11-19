#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>

int main(int argc, char* argv[])
{
    if (argc>1)
    {
        struct stat sb;
        struct group *grp;
        struct passwd *pwp;
        time_t mt;
        struct tm *mtime;
        int j, index;
        for (int i=1; i<argc; i++)
        {
            struct stat sb;
            if (stat(argv[i], &sb)==-1)
            {
                perror("Could not get information about the file");
            }
            else
            {
                if (S_ISREG(sb.st_mode))
                {
                    printf("-");
                }
                if ((sb.st_mode & 0xf000)==0x4000)
                {
                    printf("d");
                }
                if (!(S_ISREG(sb.st_mode)) && (sb.st_mode & 0x4000)!=0x4000)
                {
                    printf("?");
                }
                if (sb.st_mode & 00400)
                {
                    printf("r");
                }
                else
                {
                    printf("-");
                }
                if (sb.st_mode & 00200)
                {
                    printf("w");
                }
                else
                {
                    printf("-");
                }
                if (sb.st_mode &00100)
                {
                    printf("x");
                }
                else
                {
                    printf("-");
                }
                if (sb.st_mode & 00040)
                {
                    printf("r");
                }
                else
                {
                    printf("-");
                }
                if (sb.st_mode & 00020)
                {
                    printf("w");
                }
                else
                {
                    printf("-");
                }
                if (sb.st_mode &00010)
                {
                    printf("x");
                }
                else
                {
                    printf("-");
                }
                if (sb.st_mode & 00004)
                {
                    printf("r");
                }
                else
                {
                    printf("-");
                }
                if (sb.st_mode & 00002)
                {
                    printf("w");
                }
                else
                {
                    printf("-");
                }
                if (sb.st_mode &00001)
                {
                    printf("x");
                }
                else
                {
                    printf("-");
                }
		printf(" %3d", sb.st_nlink);
                grp=getgrgid(sb.st_gid);
                printf(" %s ", grp->gr_name);
                pwp=getpwuid(sb.st_uid);
                printf("%s ", pwp->pw_name);
                printf("%7ld ", sb.st_size);
		char month[12][4]={"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", 
"Sep", "Oct", "Nov", "Dec"};
                mt=sb.st_mtime;
                struct tm *mtime=localtime(&mt);
                printf("%s %d %2d:%02d ", month[mtime->tm_mon], mtime->tm_mday, mtime->tm_hour, mtime->tm_min);
                index=0;
                j=0;
                while (argv[i][j]!='\0')
                {
                    if (argv[i][j]=='/')
                    {
                        index=j+1;
                    }
                    j++;
                }
                printf("%s\n", &argv[i][index]);
            }
        }
    }
    exit(0);
}

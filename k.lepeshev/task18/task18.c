#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <string.h>

int main(int argc, char *argv[]){
    if(argc < 2){
        printf("Usage: %s file...\n", argv[0]);
        return 1;
    }

    int result = 0;

    mode_t masks[9] = {
        S_IRUSR, S_IWUSR, S_IXUSR,
        S_IRGRP, S_IWGRP, S_IXGRP,
        S_IROTH, S_IWOTH, S_IXOTH
    };

    size_t start=0, end=0;

    for(int i=1; i < argc; i++){
        struct stat info;

        if(lstat(argv[i], &info) == -1){
            perror(argv[i]);
            result = 1;
            continue;
        }

        char date[32];
        struct tm *modified = localtime(&info.st_mtime);

        if(modified == NULL){
            fprintf(stderr, "%s: cannot convert mod time\n", argv[i]);
            result = 1;
            continue;
        }

        if(strftime(date, sizeof(date), "%b %d %H:%M", modified) == 0){
            fprintf(stderr, "%s: cannot format mod time\n", argv[i]);
            result = 1;
            continue;
        }

        mode_t type = info.st_mode & S_IFMT;
        char file_type = '?';

        char permissions[10] = "---------";

        for(int j=0; j < 9; j++){
            if(info.st_mode & masks[j])
                permissions[j] = "rwx"[j % 3];
        }

        if(type == S_IFDIR)
            file_type = 'd';
        else if(type == S_IFREG)
            file_type = '-';

        struct passwd *owner = getpwuid(info.st_uid);
        struct group *group = getgrgid(info.st_gid);

        
        printf("%c%s %3lu ", file_type, permissions, (unsigned long)info.st_nlink);

        if(owner != NULL)
            printf("%-12s ", owner->pw_name);
        else
            printf("%-12lu ", (unsigned long)info.st_uid);

        if(group != NULL)
            printf("%-12s ", group->gr_name);
        else
            printf("%-12lu ", (unsigned long)info.st_gid);

        if(type == S_IFREG)
            printf("%10lld ", (long long)info.st_size);
        else
            printf("%10s ", "");

        const char *path = argv[i];
        end = strlen(path);

        while(end > 1 && path[end-1] == '/')
            end--;

        start = end;

        while(start > 0 && path[start-1] != '/')
            start--;

        if(start == end)
            start = end - 1;
        
        printf("%s %.*s\n", date, (int)(end - start), path + start);
    }

    return result;
}
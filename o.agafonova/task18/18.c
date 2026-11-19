
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <string.h>
#include <errno.h>

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Ошибка: укажите файл!\n");
        fprintf(stderr, "Пример: %s task18.c\n", argv[0]);
        return 1;
    }

    int errors = 0;

    for (int i = 1; i < argc; i++) {
        struct stat st;

        if (lstat(argv[i], &st) == -1) {
            perror(argv[i]);
            errors = 1;
            continue;
        }

        char type = '?';

        if (S_ISDIR(st.st_mode))
            type = 'd';
        else if (S_ISREG(st.st_mode))
            type = '-';

        char rights[10] = "---------";
        char letters[] = "rwxrwxrwx";

        mode_t flags[] = {
            S_IRUSR, S_IWUSR, S_IXUSR,
            S_IRGRP, S_IWGRP, S_IXGRP,
            S_IROTH, S_IWOTH, S_IXOTH
        };

        for (int j = 0; j < 9; j++) {
            if (st.st_mode & flags[j])
                rights[j] = letters[j];
        }

        struct passwd *user = getpwuid(st.st_uid);
        struct group *group = getgrgid(st.st_gid);

        char owner[64], groupname[64];

        if (user)
            snprintf(owner, sizeof(owner), "%s", user->pw_name);
        else
            snprintf(owner, sizeof(owner), "%u", st.st_uid);

        if (group)
            snprintf(groupname, sizeof(groupname), "%s", group->gr_name);
        else
            snprintf(groupname, sizeof(groupname), "%u", st.st_gid);

        char size[32] = "";

        if (S_ISREG(st.st_mode))
            snprintf(size, sizeof(size), "%lld",
                     (long long)st.st_size);

        char date[40];
        struct tm *t = localtime(&st.st_mtime);

        if (t == NULL) {
            fprintf(stderr, "Ошибка получения даты: %s\n", argv[i]);
            errors = 1;
            continue;
        }

        strftime(date, sizeof(date), "%Y-%m-%d %H:%M", t);

        char path[1024];
        snprintf(path, sizeof(path), "%s", argv[i]);

        int n = strlen(path);
        while (n > 1 && path[n - 1] == '/')
            path[--n] = '\0';

        char *name = strrchr(path, '/');
        name = name ? name + 1 : path;

        if (name[0] == '\0')
            name = "/";

        printf("%c%s %3lu %-12s %-12s %10s %s %s\n",
               type, rights,
               (unsigned long)st.st_nlink,
               owner, groupname, size, date, name);
    }

    return errors;
}

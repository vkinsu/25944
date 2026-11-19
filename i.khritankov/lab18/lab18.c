#define _XOPEN_SOURCE 500 // включаем объявления функций POSIX

#include <stdio.h> // printf, fprintf, fwrite, perror
#include <string.h> // strlen, strcmp, strerror
#include <errno.h> // errno
#include <inttypes.h> // intmax_t, uintmax_t и форматы целых чисел
#include <sys/types.h> // типы идентификаторов и размеров
#include <sys/stat.h> // lstat, режим и атрибуты файла
#include <pwd.h> // getpwuid
#include <grp.h> // getgrgid
#include <time.h> // localtime, strftime

static void usage(const char *program) // выводим формат запуска
{
    printf("Usage: %s PATH...\n", program);
    printf("Print file attributes, similarly to ls -ld.\n");
}

static int print_file(const char *path) // выводим атрибуты одного указанного файла
{
    struct stat info; // атрибуты файла без перехода по символической ссылке
    struct passwd *owner; // запись о владельце файла
    struct group *group; // запись о группе файла
    struct tm *modified; // местное время последнего изменения
    const mode_t bits[9] = // проверяем права владельца, группы и остальных
    {
        S_IRUSR, S_IWUSR, S_IXUSR,
        S_IRGRP, S_IWGRP, S_IXGRP,
        S_IROTH, S_IWOTH, S_IXOTH
    };
    const char symbols[] = "rwxrwxrwx"; // символы для установленных битов прав
    char permissions[11]; // тип файла, девять прав и конец строки
    char owner_number[32]; // числовой идентификатор неизвестного владельца
    char group_number[32]; // числовой идентификатор неизвестной группы
    char size[32] = ""; // размер выводим только для обычных файлов
    char date[32]; // дата и время последнего изменения файла
    const char *owner_name; // имя владельца либо его идентификатор
    const char *group_name; // имя группы либо её идентификатор
    size_t end; // конец имени без завершающих косых черт
    size_t start; // начало последнего компонента пути
    size_t i; // номер проверяемого бита прав

    if (path[0] == '\0') // пустой путь не обозначает файл
    {
        fprintf(stderr, "lab18: empty path\n");
        return 1;
    }
    if (lstat(path, &info) == -1) // получаем сведения об указанном объекте
    {
        fprintf(stderr, "lab18: %s: %s\n", path, strerror(errno));
        return 1;
    }

    permissions[0] = S_ISDIR(info.st_mode) ? 'd' :
        (S_ISREG(info.st_mode) ? '-' : '?'); // остальные типы обозначаем вопросительным знаком
    for (i = 0; i < 9; i++) // выводим только обычные биты rwx без замены на s или t
    {
        permissions[i + 1] = (info.st_mode & bits[i]) ? symbols[i] : '-';
    }
    permissions[10] = '\0'; // завершаем строку типа и прав

    owner = getpwuid(info.st_uid); // находим имя владельца по идентификатору
    snprintf(owner_number, sizeof(owner_number), "%ju", (uintmax_t)info.st_uid);
    owner_name = owner != NULL ? owner->pw_name : owner_number;
    group = getgrgid(info.st_gid); // находим имя группы по идентификатору
    snprintf(group_number, sizeof(group_number), "%ju", (uintmax_t)info.st_gid);
    group_name = group != NULL ? group->gr_name : group_number;
    if (S_ISREG(info.st_mode)) // для каталогов и других объектов оставляем поле пустым
    {
        snprintf(size, sizeof(size), "%jd", (intmax_t)info.st_size);
    }

    modified = localtime(&info.st_mtime); // используем время изменения содержимого файла
    if (modified == NULL || strftime(date, sizeof(date), "%Y-%m-%d %H:%M:%S", modified) == 0)
    {
        fprintf(stderr, "lab18: %s: cannot format modification time\n", path);
        return 1;
    }

    end = strlen(path); // начинаем поиск имени с конца пути
    while (end > 1 && path[end - 1] == '/') // убираем завершающие черты, сохраняя корень
    {
        end--;
    }
    start = end; // ищем начало последнего компонента
    while (start > 0 && path[start - 1] != '/')
    {
        start--;
    }
    if (end == 1 && path[0] == '/') // имя корневого каталога состоит из одной черты
    {
        start = 0;
    }

    printf("%s %4ju %-12s %-12s %10s %s ", permissions,
        (uintmax_t)info.st_nlink, owner_name, group_name, size, date); // выравниваем поля атрибутов
    fwrite(path + start, 1, end - start, stdout); // печатаем только имя без родительских каталогов
    putchar('\n'); // каждый объект занимает отдельную строку
    return 0;
}

int main(int argc, char *argv[])
{
    int i; // номер аргумента командной строки
    int status = 0; // сохраняем ошибку любого обработанного пути

    if (argc < 2) // без путей сообщаем формат запуска и возвращаем ошибку
    {
        usage(argv[0]);
        return 1;
    }
    if (argc == 2 && (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0))
    {
        usage(argv[0]); // отдельный запрос справки завершается успешно
        return 0;
    }
    for (i = 1; i < argc; i++) // продолжаем обработку после ошибок отдельных файлов
    {
        if (print_file(argv[i]) != 0)
        {
            status = 1;
        }
    }
    if (fflush(stdout) == EOF) // проверяем ошибки записи, включая буферизованный вывод
    {
        perror("lab18: stdout");
        status = 1;
    }
    return status;
}

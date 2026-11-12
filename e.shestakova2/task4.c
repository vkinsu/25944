#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>

extern char *readline(const char *prompt);
extern void add_history(const char *line);

struct Node
{
    char *text;
    struct Node *next;
};

static char *read_text(int terminal, int *status)
{
    char buffer[256];
    char *text = NULL;
    char *grown;
    size_t length = 0;
    size_t part;

    if (terminal)
    {
        text = readline(NULL);
        if (text == NULL)
        {
            return NULL;
        }

        if (text[0] != '\0' && text[0] != '.')
        {
            add_history(text);
        }

        length = strlen(text);
        if (length > SIZE_MAX - 2)
        {
            fprintf(stderr, "Line is too long\n");
            free(text);
            *status = 1;
            return NULL;
        }

        grown = realloc(text, length + 2);
        if (grown == NULL)
        {
            perror("realloc");
            free(text);
            *status = 1;
            return NULL;
        }

        grown[length] = '\n';
        grown[length + 1] = '\0';
        return grown;
    }

    while (fgets(buffer, sizeof(buffer), stdin) != NULL)
    {
        part = strlen(buffer);

        if (part == 0 || length > SIZE_MAX - part - 1)
        {
            fprintf(stderr, "Invalid or too long text line\n");
            free(text);
            *status = 1;
            return NULL;
        }

        grown = realloc(text, length + part + 1);
        if (grown == NULL)
        {
            perror("realloc");
            free(text);
            *status = 1;
            return NULL;
        }

        text = grown;
        memcpy(text + length, buffer, part + 1);
        length += part;

        if (buffer[part - 1] == '\n')
        {
            break;
        }
    }

    if (ferror(stdin))
    {
        perror("fgets");
        free(text);
        *status = 1;
        return NULL;
    }

    return text;
}

int main(void)
{
    struct Node *head = NULL;
    struct Node *tail = NULL;
    struct Node *node;
    char *text;
    int terminal = isatty(STDIN_FILENO);
    int status = 0;

    while ((text = read_text(terminal, &status)) != NULL)
    {
        if (text[0] == '.')
        {
            free(text);
            break;
        }

        node = malloc(sizeof(*node));
        if (node == NULL)
        {
            perror("malloc");
            free(text);
            status = 1;
            break;
        }

        node->text = text;
        node->next = NULL;

        if (head == NULL)
        {
            head = node;
        }
        else
        {
            tail->next = node;
        }

        tail = node;
    }

    while (head != NULL)
    {
        node = head;

        if (status == 0)
        {
            printf("%s", node->text);
        }

        head = head->next;
        free(node->text);
        free(node);
    }

    return status;
}

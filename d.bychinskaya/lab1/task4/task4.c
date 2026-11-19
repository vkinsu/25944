#include <stdio.h>
#include <stdlib.h>
#include <locale.h>
#include <readline/readline.h>
#include <readline/history.h>

struct Node {
    char *text;
    struct Node *next;
};

static void free_list(struct Node *head)
{
    struct Node *next;

    while (head != NULL) {
        next = head->next;
        free(head->text);
        free(head);
        head = next;
    }
}

int main(void)
{
    struct Node *head = NULL;
    struct Node *tail = NULL;
    struct Node *node;
    char *line;
    int status = EXIT_SUCCESS;

    setlocale(LC_ALL, "");

    printf("Enter lines. Start a line with '.' to finish.\n");

    for (;;) {
        line = readline("> ");

        /* Ctrl+D завершает ввод. */
        if (line == NULL)
            break;

        if (line[0] == '.') {
            free(line);
            break;
        }

        node = malloc(sizeof(*node));
        if (node == NULL) {
            fprintf(stderr, "Cannot allocate memory\n");
            free(line);
            status = EXIT_FAILURE;
            break;
        }

        node->text = line;
        node->next = NULL;

        if (head == NULL)
            head = node;
        else
            tail->next = node;

        tail = node;

        if (line[0] != '\0')
            add_history(line);
    }

    if (status == EXIT_SUCCESS) {
        printf("\nSaved lines:\n");

        for (node = head; node != NULL; node = node->next)
            printf("%s\n", node->text);
    }

    free_list(head);
    clear_history();

    return status;
}

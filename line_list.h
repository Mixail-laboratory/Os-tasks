#ifndef TASK5_6_LINE_LIST_H
#define TASK5_6_LINE_LIST_H

#include <sys/types.h>

struct line {
    off_t offset;
    off_t len;
};

struct node {
    struct line line;
    struct node *next;
};

struct list {
    struct node *head;
    struct node *last;
};

void init_list(struct list *list);

void free_list(struct list *list);

struct node *get_node(struct node *head, size_t offset);

int add_line(struct list *list, struct line *line);

#endif

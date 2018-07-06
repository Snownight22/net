#ifndef _LISt_H_ 
#define _LIST_H_

#define offset_of(TYPE, MEMBER) ((size_t)&((TYPE*)0)->MEMBER)

#define container_of(ptr, type, member)                 \
({                                                      \
    const typeof(((type *)0)->member) *__mptr = ptr;    \
    (type *)((char *)__mptr - offset_of(type, member)); \
})

typedef struct list_entry
{
    struct list_entry* prev;
    struct list_entry* next;
}stListEntry;

typedef struct list_head
{
    stListEntry *first;
    stListEntry *last;
}stListHead;

#define LIST_INIT(head)   \
    (head)->first = NULL; \
    (head)->last = NULL;  

#define LIST_INSERT(head, node)       \
    (node)->prev = NULL;              \
    (node)->next = (head)->first;     \
    if (NULL == (head)->first)        \
    {                                 \
        (head)->first = (node);       \
        (head)->last = (node);        \
    }                                 \
    else                              \
    {                                 \
        (head)->first->prev = (node); \
        (head)->first = (node);       \
    }

#define LIST_INSERT_TAIL(head, node)  \
    (node)->prev = (head)->last;      \
    (node)->next = NULL;              \
    if (NULL == (head)->last)         \
    {                                 \
        (head)->first = (node);       \
        (head)->last = (node);        \
    }                                 \
    else                              \
    {                                 \
        (head)->last->next = (node);  \
        (head)->last = (node);        \
    }

#define LIST_DELETE(head, node)            \
    if ((head)->first == (node))           \
        (head)->first = (node)->next;      \
    else                                   \
        (node)->prev->next = (node)->next; \
    if ((head)->last == (node))            \
        (head)->last = (node)->prev;       \
    else                                   \
        (node)->next->prev = (node)->prev; 

#define LIST_FOREACH(head, entry)    \
    for((entry) = (head)->first; NULL != (entry); (entry) = (entry)->next)

#define LIST_FOREACH_NEXT(head, entry, next)                     \
    for (                                                        \
        (next) = (((entry) = (head)->first)?(entry)->next:NULL); \
        NULL != (entry);                                         \
        (entry) = (next), (next) = ((next)?(next)->next:NULL)    \
        )



#endif

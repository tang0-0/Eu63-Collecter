#ifndef __TY_LIST_H__
#define __TY_LIST_H__


#define ty_container_of(ptr, type, member) \
    ((type *)((char *)(ptr) - (unsigned long)(&((type *)0)->member)))


struct ty_slist_node
{
    struct ty_slist_node *next;
};
typedef struct ty_slist_node ty_slist_t;

struct ty_list_node
{
    struct ty_list_node *next;
    struct ty_list_node *prev;
};
typedef struct ty_list_node ty_list_t;

static inline void ty_slist_init(ty_slist_t *l)
{
    l->next = 0;
}

static inline void ty_slist_append(ty_slist_t *h, ty_slist_t *n)
{
    struct ty_slist_node *node;

    node = h;
    while (node->next) node = node->next;

    node->next = n;
    n->next = 0;
}

static inline void ty_slist_insert(ty_slist_t *l, ty_slist_t *n)
{
    n->next = l->next;
    l->next = n;
}

static inline ty_slist_t *ty_slist_remove(ty_slist_t *h, ty_slist_t *n)
{
    struct ty_slist_node *node = h;
    while (node->next && node->next != n) node = node->next;

    if (node->next != (ty_slist_t *)0) node->next = node->next->next;

    return h;
}

static inline unsigned int ty_slist_get_len(const ty_slist_t *l)
{
    unsigned int len = 0;
    const ty_slist_t *list = l->next;

    while (list != 0)
    {
        list = list->next;
        len ++;
    }

    return len;
}

static inline int ty_slist_is_empty(ty_slist_t *l)
{
    return (l->next == 0);
}

static inline ty_slist_t *ty_slist_get_first(ty_slist_t *h)
{
    return h->next;
}

static inline ty_slist_t *ty_slist_get_last(ty_slist_t *l)
{
    while (l->next) l = l->next;

    return l;
}

#define ty_slist_for_each(pos, head) \
    for (pos = (head)->next; pos != 0; pos = pos->next)

#define ty_slist_entry(node, type, member) \
    ty_container_of(node, type, member)

#define ty_slist_for_each_entry(pos, head, member) \
    for (pos = ty_slist_entry((head)->next, typeof(*pos), member); \
         &pos->member != (0); \
         pos = ty_slist_entry(pos->member.next, typeof(*pos), member))

#define ty_slist_get_first_entry(ptr, type, member) \
    ty_slist_entry((ptr)->next, type, member)

#define ty_slist_get_last_entry(ptr, type, member) \
    ty_slist_entry(ty_slist_get_last(ptr), type, member)


static inline void ty_list_init(ty_list_t *l)
{
    l->next = l->prev = 0;
}

static inline void ty_list_append_head(ty_list_t *h, ty_list_t *n)
{
    if ((n->next = h->next) != 0)
    {
        h->next->prev = n;
    }

    h->next = n;
    n->prev = h;
}

static inline void ty_list_append_tail(ty_list_t *h, ty_list_t *n)
{
    while (h->next)
    {
        h = h->next;
    }

    h->next = n;
    n->prev = h;
}

static inline void ty_list_insert_after(ty_list_t *l, ty_list_t *n)
{
    l->next->prev = n;
    n->next = l->next;

    l->next = n;
    n->prev = l;
}

static inline void ty_list_insert_before(ty_list_t *l, ty_list_t *n)
{
    l->prev->next = n;
    n->prev = l->prev;

    l->prev = n;
    n->next = l;
}

static inline void ty_list_remove(ty_list_t *n)
{
    n->next->prev = n->prev;
    n->prev->next = n->next;

    n->next = n->prev = 0;
}

static inline int ty_list_is_empty(const ty_list_t *l)
{
    return (l->next == 0);
}

static inline unsigned int ty_list_get_len(const ty_list_t *l)
{
    unsigned int len = 0;
    const ty_list_t *p = l;

    while (p->next != 0)
    {
        p = p->next;
        len ++;
    }

    return len;
}

#define ty_list_for_each(pos, head) \
    for (pos = (head)->next; pos != (0); pos = pos->next)

#define ty_list_for_each_safe(pos, n, head) \
    for (pos = (head)->next, n = pos->next; pos != (0); \
        pos = n, n = pos->next)

#define ty_list_entry(node, type, member) \
    ty_container_of(node, type, member)

#define ty_list_for_each_entry(tpos, pos, head, member) \
    for (pos = (head)->next; \
         pos && ({ tpos = ty_list_entry(pos, typeof(*tpos), member); 1 ;}); \
         pos = pos->next)

#define ty_list_for_each_entry_safe(tpos, pos, n, head, member) \
    for (pos = (head)->next; \
         pos && ({ n = pos->next; 1;}) && ({ tpos = ty_list_entry(pos, typeof(*tpos), member); 1 ;}); \
         pos = n)


#endif


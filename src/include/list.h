#ifndef XOS_LIST_H
#define XOS_LIST_H

#include <stdint.h>


#define element_offset(type, member) (uint32_t)(&((type *)0)->member)
//通过 member成员获取其所属的type结构体指针
#define element_entry(type, member, ptr) (type *)((uint32_t)ptr - element_offset(type, member))

#define element_node_offset(type, node, key) ((int)(&((type *)0)->key) - (int)(&((type *)0)->node))

#define element_node_key(node, offset) (*(int *)((int)node + offset))

typedef struct list_node_t {
    struct list_node_t *prev;
    struct list_node_t *next;
} list_node_t;

//链表
typedef struct list_t {
    list_node_t head;
    list_node_t tail;
} list_t;

//线性数组
typedef struct array_t {
    uint32_t length;
    void *array;
} array_t;

void list_init(list_t *list);

//anchor结点前插入node
void list_insert_before(list_node_t *anchor, list_node_t *node);
void list_insert_after(list_node_t *anchor, list_node_t *node);

void list_push(list_t *list, list_node_t *node);
//弹出头结点后的节点
list_node_t *list_pop(list_t* list);

void list_pushback(list_t *list, list_node_t *node);
list_node_t *list_popback(list_t *list);

bool list_search(list_t *list, list_node_t *node);
void list_remove(list_node_t *node);

bool list_empty(list_t *list);
uint32_t list_size(list_t *list);


//链表插入排序
void list_insert_sort(list_t *list, list_node_t *node, int offset);


#endif
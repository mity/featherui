/*
 * libex
 * <http://github.com/mity/libex>
 *
 * Copyright (c) 2018-2026 Martin Mitáš
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#ifndef EX_LIST_H
#define EX_LIST_H

#ifdef __cplusplus
extern "C" {
#endif


#if defined offsetof
    #define LIST_OFFSETOF__(type, member)   offsetof(type, member)
#elif defined __GNUC__ && __GNUC__ >= 4
    #define LIST_OFFSETOF__(type, member)   __builtin_offsetof(type, member)
#else
    #define LIST_OFFSETOF__(type, member)   ((size_t) &((type*)0)->member)
#endif


/* This header implements few simple intrusive linked lists:
 *
 *  - Double-linked lists (List)
 *  - Single-linked lists (SList)
 *  - Single-linked lists which also the tail, aka queue (QList)
 *
 * The word intrusive means our node structures (ListNode, SListNode or
 * QListNode) don't hold any data on their own. Instead, you are supposed to
 * embed them in your structure.
 *
 * Given the simplicity of all operations, all functions are inline.
 *
 * The naming of all operations follows a common pattern <listtype>_<operation>,
 * so e.g. list_next() is used to advance to the following node in the doubly
 * linked list and slist_next() in single-linked list.
 *
 * For all the manipulations with the lists, you are supposed to use pointer
 * to our node structures (ListNode, SListNode or QListNode). To retrieve
 * the payload data, use the macro LIST_DATA, SLIST_DATA or QUEUE_DATA as
 * appropriate (all of these are actually just a synonym for the wildly used
 * container_of() macro.)
 *
 * Note that our lists may use the common little trick and avoid using NULL as
 * the end-of-list sentinel. Instead the first and last nodes point to the
 * dummy structure representing the list as a whole. This simplifies many
 * insert/remove operations because it mitigates the need for branches.
 *
 * The cost of that trick is the caller may not test whether he has reached
 * the end of the list with NULL. For the sake of simplicity we provide the
 * function list_end(), which can be used similarly as std::vector::end() in
 * C++ (both in forward as well as backward iteration).
 *
 * Hence, the typical iteration over e.g. the double-linked list may look like
 * this:
 *
 * ````
 * typedef struct MyStruct {
 *    ...   // Some data
 *
 *    // The embedded list node structure:
 *    ListNode ListNode;
 *
 *    ...   // Some more data
 * } MyStruct;
 *
 *
 * static void
 * walk_my_list(List* list)
 * {
 *     ListNode* node;
 *     MyStruct* data_payload;
 *
 *     for(node = list_head(list); node != list_end(list); node = list_next(node)) {
 *         // Retrieve the data pay load from the node:
 *         data_payload = LIST_DATA(node, MyStruct, ListNode);
 *
 *         // Process the data payload as desired.
 *         ...
 *     }
 * }
 * ````
 *
 * An overview of available operations, depending on the list type:
 *
 *                              | List   | SList  | QList
 * -----------------------------|--------|--------|-------
 * <listtype>_init()            | yes    | yes    | yes
 * <listtype>_is_empty()        | yes    | yes    | yes
 * <listtype>_head()            | yes    | yes    | yes
 * <listtype>_tail()            | yes    |        | yes
 * <listtype>_next()            | yes    | yes    | yes
 * <listtype>_prev()            | yes    |        |
 * <listtype>_end()             | yes    | yes    | yes
 * <listtype>_insert_after()    | yes    | yes    |
 * <listtype>_insert_before()   | yes    |        |
 * <listtype>_append()          | yes    |        | yes
 * <listtype>_prepend()         | yes    | yes    | yes
 * <listtype>_remove()          | yes    | yes(1) | yes(1)
 * <listtype>_remove_head()     | yes    | yes    | yes
 * <listtype>_remove_tail()     | yes    |        |
 *
 * Notes:
 *  (1): The caller has to additionally provide pointer to the _previous_ node.
 */


/*********************************
 *** List (doubly linked list) ***
 *********************************/

/* List node structure. Treat as opaque.
 */
typedef struct ListNode {
    struct ListNode* p;    /* prev */
    struct ListNode* n;    /* next */
} ListNode;


/* List structure. Treat as opaque.
 */
typedef struct List {
    struct ListNode main;
} List;


/* Macro for getting pointer to the structure holding list node data.
 */
#define LIST_DATA(node_ptr, type, member)  \
                ((type*)((char*)(node_ptr) - LIST_OFFSETOF__(type, member)))


/* The list has to be initialized before it is used by any other function.
 */
static inline void list_init(List* list)
        { list->main.p = list->main.n = &list->main; }


/* Check whether the list is empty or not.
 */
static inline int list_is_empty(const List* list)
        { return (list->main.n == &list->main); }


/* Iterating the list.
 */
static inline ListNode* list_head(const List* list)         { return list->main.n; }
static inline ListNode* list_tail(const List* list)         { return list->main.p; }
static inline ListNode* list_prev(const ListNode* node)     { return node->p; }
static inline ListNode* list_next(const ListNode* node)     { return node->n; }
static inline const ListNode* list_end(const List* list)    { return &list->main; }

/* Add the given node into the list.
 *
 * Note that any node can be only in one list at any given time. If you
 * attempt to add the node into multiple lists (or multiple times into the
 * same list), the result is undefined.
 */
static inline void list_insert_after(List* list, ListNode* node_where, ListNode* node)
        { (void)list; node->p = node_where; node->n = node_where->n;
            node_where->n = node; node->n->p = node; }
static inline void list_insert_before(List* list, ListNode* node_where, ListNode* node)
        { (void)list; node->p = node_where->p; node->n = node_where;
            node_where->p = node; node->p->n = node; }
static inline void list_append(List* list, ListNode* node)
        { list_insert_before(list, &list->main, node); }
static inline void list_prepend(List* list, ListNode* node)
        { list_insert_after(list, &list->main, node); }

/* Disconnect the given node from its list.
 */
static inline void list_remove(List* list, ListNode* node)
        { ((void) list); node->p->n = node->n; node->n->p = node->p; }
static inline void list_remove_head(List* list)
        { list_remove(list, list->main.n); }
static inline void list_remove_tail(List* list)
        { list_remove(list, list->main.p); }


/* Convenient macro for walking over complete list.
 */
#define LIST_FOR_EACH(list, node)  \
        for((node) = list_head((list)); (node) != list_end((list)); (node) = list_next((node)))


/**********************************
 *** SList (single-linked list) ***
 **********************************/

/* List node structure. Treat as opaque.
 */
typedef struct SListNode {
    struct SListNode* n;       /* next */
} SListNode;


/* List structure. Treat as opaque.
 */
typedef struct SList {
    struct SListNode main;
} SList;


/* Macro for getting pointer to the structure holding list node data.
 */
#define SLIST_DATA(node_ptr, type, member)  \
                ((type*)((char*)(node_ptr) - LIST_OFFSETOF__(type, member)))


/* The list has to be initialized before it is used by any other function.
 */
static inline void slist_init(SList* list)
        { list->main.n = &list->main; }


/* Check whether the list is empty or not.
 */
static inline int slist_is_empty(const SList* list)
        { return (list->main.n == &list->main); }


/* Iterating the list.
 */
static inline SListNode* slist_head(const SList* list)         { return list->main.n; }
static inline SListNode* slist_next(const SListNode* node)    { return node->n; }
static inline const SListNode* slist_end(const SList* list)    { return &list->main; }

/* Add the given node into the list.
 *
 * Note that any node can be only in one list at any given time. If you
 * attempt to add the node into multiple lists (or multiple times into the
 * same list), the result is undefined.
 */
static inline void slist_insert_after(SList* list, SListNode* node_where, SListNode* node)
        { (void)list; node->n = node_where->n; node_where->n = node; }
static inline void slist_prepend(SList* list, SListNode* node)
        { slist_insert_after(list, &list->main, node); }

/* Disconnect the given node from its list.
 */
static inline void slist_remove(SList* list, SListNode* node_prev, SListNode* node)
        { (void)list; node_prev->n = node->n; }
static inline void slist_remove_head(SList* list)
        { slist_remove(list, &list->main, list->main.n); }


/* Convenient macro for walking over complete list.
 */
#define SLIST_FOR_EACH(list, node)  \
        for((node) = slist_head((list)); (node) != slist_end((list)); (node) = slist_next((node)))


/*****************************************************
 *** QList (queue or single-linked list with tail) ***
 *****************************************************/

/* List node structure. Treat as opaque.
 */
typedef struct QListNode {
    struct QListNode* n;       /* next */
} QListNode;


/* List structure. Treat as opaque.
 */
typedef struct QList {
    struct QListNode main;
    struct QListNode* tail;
} QList;


/* Macro for getting pointer to the structure holding list node data.
 */
#define QLIST_DATA(node_ptr, type, member)  \
                ((type*)((char*)(node_ptr) - LIST_OFFSETOF__(type, member)))

/* The list has to be initialized before it is used by any other function.
 */
static inline void qlist_init(QList* list)
        { list->tail = list->main.n = &list->main; }

/* Check whether the list is empty or not.
 */
static inline int qlist_is_empty(const QList* list)
        { return (list->main.n == &list->main); }

/* Iterating the list.
 */
static inline QListNode* qlist_head(const QList* list)         { return list->main.n; }
static inline QListNode* qlist_tail(const QList* list)         { return list->tail; }
static inline QListNode* qlist_next(const QListNode* node)    { return node->n; }
static inline const QListNode* qlist_end(const QList* list)    { return &list->main; }

/* Add the given node into the list.
 *
 * Note that any node can be only in one list at any given time. If you
 * attempt to add the node into multiple lists (or multiple times into the
 * same list), the result is undefined.
 */
static inline void qlist_insert_after(QList* list, QListNode* node_where, QListNode* node)
        { node->n = node_where->n; node_where->n = node;
          if(list->tail == node_where) list->tail = node; }
static inline void qlist_append(QList* list, QListNode* node)
        { node->n = &list->main; list->tail->n = node; list->tail = node; }
static inline void qlist_prepend(QList* list, QListNode* node)
        { qlist_insert_after(list, &list->main, node); }

/* Disconnect the given node from its list.
 */
static inline void qlist_remove(QList* list, QListNode* node_prev, QListNode* node)
        { node_prev->n = node->n;
          if(list->tail == node) list->tail = node_prev; }
static inline void qlist_remove_head(QList* list)
        { qlist_remove(list, &list->main, list->main.n); }


/* Convenient macro for walking over complete list.
 */
#define QLIST_FOR_EACH(list, node)  \
        for((node) = qlist_head((list)); (node) != qlist_end((list)); (node) = qlist_next((node)))


#ifdef __cplusplus
}  /* extern "C" { */
#endif

#endif  /* EX_LIST_H */

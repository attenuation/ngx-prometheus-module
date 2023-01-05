#ifndef PROM_LIST_T_H
#define PROM_LIST_T_H
#include <stdlib.h>
#include "ngx_core.h"

typedef enum { PROM_LESS = -1, PROM_EQUAL = 0, PROM_GREATER = 1 } prom_linked_list_compare_t;

/**
 * @brief API PRIVATE Frees an item in a prom_linked_list_node
 */
typedef void (*prom_linked_list_free_item_fn)(void *);

/**
 * @brief API PRIVATE Compares two items within a prom_linked_list
 */
typedef prom_linked_list_compare_t (*prom_linked_list_compare_item_fn)(void *item_a, void *item_b);

/**
 * @brief API PRIVATE A struct containing a generic item, represented as a void pointer, and next, a pointer to the
 * next prom_linked_list_node*
 */
typedef struct prom_linked_list_node {
  struct prom_linked_list_node *next;
  void *item;
  ngx_slab_pool_t *shpool;
} prom_linked_list_node_t;

/**
 * @brief API PRIVATE A linked list comprised of prom_linked_list_node* instances
 */
typedef struct prom_linked_list {
  prom_linked_list_node_t *head;
  prom_linked_list_node_t *tail;
  size_t size;
  prom_linked_list_free_item_fn free_fn;
  prom_linked_list_compare_item_fn compare_fn;
  ngx_slab_pool_t *shpool;
} prom_linked_list_t;

/**
 * @brief API PRIVATE Returns a pointer to a prom_linked_list
 */
prom_linked_list_t *prom_linked_list_new(ngx_slab_pool_t *shpool);

/**
 * @brief API PRIVATE removes all nodes from the given prom_linked_list *
 */
int prom_linked_list_purge(prom_linked_list_t *self);

/**
 * @brief API PRIVATE Destroys a prom_linked_list
 */
int prom_linked_list_destroy(prom_linked_list_t *self);

/**
 * @brief API PRIVATE Append an item to the back of the list
 */
int prom_linked_list_append(prom_linked_list_t *self, void *item);

/**
 * @brief API PRIVATE Push an item onto the front of the list
 */
int prom_linked_list_push(prom_linked_list_t *self, void *item);

/**
 * @brief API PRIVATE Pop the first item off of the list
 */
void *prom_linked_list_pop(prom_linked_list_t *self);

/**
 * @brief API PRIVATE Returns the item at the head of the list or NULL if not present
 */
void *prom_linked_list_first(prom_linked_list_t *self);

/**
 * @brief API PRIVATE Returns the item at the tail of the list or NULL if not present
 */
void *prom_linked_list_last(prom_linked_list_t *self);

/**
 * @brief API PRIVATE Removes an item from the linked list
 */
int prom_linked_list_remove(prom_linked_list_t *self, void *item);

/**
 * @brief API PRIVATE Compares two items within a linked list
 */
prom_linked_list_compare_t prom_linked_list_compare(prom_linked_list_t *self, void *item_a, void *node_b);

/**
 * @brief API PRIVATE Get the size
 */
size_t prom_linked_list_size(prom_linked_list_t *self);

/**
 * @brief API PRIVATE Set the free_fn member on prom_linked_list
 */
int prom_linked_list_set_free_fn(prom_linked_list_t *self, prom_linked_list_free_item_fn free_fn);

/**
 * @brief API PRIVATE Set the compare_fn member on the prom_linked_list
 */
int prom_linked_list_set_compare_fn(prom_linked_list_t *self, prom_linked_list_compare_item_fn compare_fn);

/**
 * API PRIVATE
 * @brief does nothing
 */
void prom_linked_list_no_op_free(void *item);

#endif  // PROM_LIST_T_H
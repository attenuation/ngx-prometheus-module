#ifndef PROM_MAP_T_H
#define PROM_MAP_T_H

#include "ngx_core.h"
#include <pthread.h>


// Private
#include "prom_linked_list.h"

typedef void (*prom_map_node_free_value_fn)(void *);

typedef struct prom_map_node {
  const char *key;
  void *value;
  ngx_slab_pool_t *shpool;
  prom_map_node_free_value_fn free_value_fn;
} prom_map_node_t;

typedef struct prom_map {
  size_t size;                /**< contains the size of the map */
  size_t max_size;            /**< stores the current max_size */
  prom_linked_list_t *keys;   /**< linked list containing containing all keys present */
  prom_linked_list_t **addrs; /**< Sequence of linked lists. Each list contains nodes with the same index */
  ngx_atomic_t       rwlock;
  prom_map_node_free_value_fn free_value_fn;
  ngx_slab_pool_t *shpool;
} prom_map_t;


prom_map_t *prom_map_new(ngx_slab_pool_t *shpool);

int prom_map_set_free_value_fn(prom_map_t *self, prom_map_node_free_value_fn free_value_fn);

void *prom_map_get(prom_map_t *self, const char *key);

int prom_map_set(prom_map_t *self, const char *key, void *value);

int prom_map_delete(prom_map_t *self, const char *key);

int prom_map_destroy(prom_map_t *self);

size_t prom_map_size(prom_map_t *self);

prom_map_node_t *prom_map_node_new(ngx_slab_pool_t *shpool, const char *key, void *value, prom_map_node_free_value_fn free_value_fn);

#endif  // PROM_MAP_T_H

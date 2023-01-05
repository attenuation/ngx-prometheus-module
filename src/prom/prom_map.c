#include "prom_map.h"

#define PROM_MAP_INITIAL_SIZE 32

static void destroy_map_node_value_no_op(void *value) {}


prom_map_node_t *prom_map_node_new(ngx_slab_pool_t *shpool, const char *key, void *value, prom_map_node_free_value_fn free_value_fn) {
    prom_map_node_t *self = ngx_slab_calloc(shpool, sizeof(prom_map_node_t));
    if (self == NULL) {
        return NULL;
    }
    self->shpool = shpool;

    if (key == NULL) {
        return NULL;
    }

    self->key = ngx_slab_calloc(shpool, ngx_strlen(key));
    if (self->key == NULL) {
        return NULL;
    }
    ngx_memcpy(self->key, key, ngx_strlen(key));

    self->value = value;
    self->free_value_fn = free_value_fn;
    return self;
}

int prom_map_node_destroy(prom_map_node_t *self) {
    if (self == NULL) return 0;
    ngx_slab_free(self->shpool, self->key);
    self->key = NULL;
    if (self->value != NULL) (*self->free_value_fn)(self->value);
    self->value = NULL;
    ngx_slab_free(self->shpool, self);
    self = NULL;
    return 0;
}

void prom_map_node_free(void *item) {
    prom_map_node_t *map_node = (prom_map_node_t *)item;
    prom_map_node_destroy(map_node);
}

prom_linked_list_compare_t prom_map_node_compare(void *item_a, void *item_b) {
    prom_map_node_t *map_node_a = (prom_map_node_t *)item_a;
    prom_map_node_t *map_node_b = (prom_map_node_t *)item_b;

    return strcmp(map_node_a->key, map_node_b->key);
}

prom_map_t *prom_map_new(ngx_slab_pool_t *shpool)
{  
    int r = 0;

    prom_map_t *self = (prom_map_t *)ngx_slab_calloc(shpool, sizeof(prom_map_t));
    if (self == NULL) {
        return NULL;
    }

    self->size = 0;
    self->max_size = PROM_MAP_INITIAL_SIZE;
    self->shpool = shpool;

    self->keys = prom_linked_list_new(shpool);
    if (self->keys == NULL) return NULL;

    // These each key will be allocated once by prom_map_node_new and used here as well to save memory. With that said
    // we will only have to deallocate each key once. That will happen on prom_map_node_destroy.
    r = prom_linked_list_set_free_fn(self->keys, prom_linked_list_no_op_free);
    if (r) {
        prom_map_destroy(self);
        return NULL;
    }

    self->addrs = ngx_slab_calloc(shpool, sizeof(prom_linked_list_t) * self->max_size);
    self->free_value_fn = destroy_map_node_value_no_op;

    for (int i = 0; i < self->max_size; i++) {
        self->addrs[i] = prom_linked_list_new(shpool);
        r = prom_linked_list_set_free_fn(self->addrs[i], prom_map_node_free);
        if (r) {
            prom_map_destroy(self);
            return NULL;
        }
        r = prom_linked_list_set_compare_fn(self->addrs[i], prom_map_node_compare);
        if (r) {
            prom_map_destroy(self);
            return NULL;
        }
    }

    return self;
}

int prom_map_destroy(prom_map_t *self) {
    if (self == NULL) return 1;
    int r = 0;
    int ret = 0;

    r = prom_linked_list_destroy(self->keys);
    if (r) ret = r;
    self->keys = NULL;

    for (size_t i = 0; i < self->max_size; i++) {
        r = prom_linked_list_destroy(self->addrs[i]);
        if (r) ret = r;
        self->addrs[i] = NULL;
    }
    ngx_slab_free(self->shpool, self->addrs);
    self->addrs = NULL;

    ngx_slab_free(self->shpool, self);
    self = NULL;

    return ret;
}

static size_t prom_map_get_index_internal(const char *key, size_t *size, size_t *max_size) {
  size_t index;
  size_t a = 31415, b = 27183;
  for (index = 0; *key != '\0'; key++, a = a * b % (*max_size - 1)) {
    index = (a * index + *key) % *max_size;
  }
  return index;
}

/**
 * @brief API PRIVATE hash function that returns an array index from the given key and prom_map.
 *
 * The algorithm is based off of Horner's method. In a simpler version, you set the return value to 0. Next, for each
 * character in the string, you add the integer value of the current character to the product of the prime number and
 * the current return value, set the result to the return value, then finally return the return value.
 *
 * In this version of the algorithm, we attempt to achieve a probabily of key to index conversion collisions to
 * 1/M (with M being the max_size of the map). This optimizes dispersion and consequently, evens out the performance
 * for gets and sets for each item. Instead of using a fixed prime number, we generate a coefficient for each iteration
 * through the loop.
 *
 * Reference:
 *   * Algorithms in C: Third Edition by Robert Sedgewick, p579
 */
size_t prom_map_get_index(prom_map_t *self, const char *key) {
  return prom_map_get_index_internal(key, &self->size, &self->max_size);
}

static void *prom_map_get_internal(const char *key, size_t *size, size_t *max_size, prom_linked_list_t *keys,
                                   prom_linked_list_t **addrs, prom_map_node_free_value_fn free_value_fn) {
  size_t index = prom_map_get_index_internal(key, size, max_size);
  prom_linked_list_t *list = addrs[index];

  for (prom_linked_list_node_t *current_node = list->head; current_node != NULL; current_node = current_node->next) {
    prom_map_node_t *current_map_node = (prom_map_node_t *)current_node->item;
    if(strcmp(current_map_node->key, key) == 0) {
      return current_map_node->value;
    }
  }

  return NULL;
}

void *prom_map_get(prom_map_t *self, const char *key) {
    if (self == NULL) return NULL;
    int r = 0;
    ngx_rwlock_rlock(&self->rwlock);
    void *payload =
        prom_map_get_internal(key, &self->size, &self->max_size, self->keys, self->addrs, self->free_value_fn);
    ngx_rwlock_unlock(&self->rwlock);
    return payload;
}

static int prom_map_set_internal(const char *key, void *value, size_t *size, size_t *max_size, prom_linked_list_t *keys,
                                 prom_linked_list_t **addrs, prom_map_node_free_value_fn free_value_fn,
                                 int destroy_current_value) {
    prom_map_node_t *map_node = prom_map_node_new(addrs[0]->shpool, key, value, free_value_fn);
    if (map_node == NULL) return 1;

    size_t index = prom_map_get_index_internal(key, size, max_size);
    prom_linked_list_t *list = addrs[index];
    for (prom_linked_list_node_t *current_node = list->head; current_node != NULL; current_node = current_node->next) {
        prom_map_node_t *current_map_node = (prom_map_node_t *)current_node->item;
        prom_linked_list_compare_t result = prom_linked_list_compare(list, current_map_node, map_node);
        if (result == PROM_EQUAL) {
            if (destroy_current_value) {
                free_value_fn(current_map_node->value);
                current_map_node->value = NULL;
            }
            ngx_slab_free(current_map_node->shpool, current_map_node->key);
            current_map_node->key = NULL;
            ngx_slab_free(current_map_node->shpool, current_map_node);
            current_map_node = NULL;
            current_node->item = map_node;
            return 0;
        }
    }
    prom_linked_list_append(list, map_node);
    prom_linked_list_append(keys, (char *)map_node->key);
    (*size)++;
    return 0;
}

int prom_map_ensure_space(prom_map_t *self) {
    if (self == NULL) return 1;
    int r = 0;

    if (self->size <= self->max_size / 2) {
        return 0;
    }

    // Increase the max size
    size_t new_max = self->max_size * 2;
    size_t new_size = 0;

    // Create a new list of keys
    prom_linked_list_t *new_keys = prom_linked_list_new(self->shpool);
    if (new_keys == NULL) return 1;

    r = prom_linked_list_set_free_fn(new_keys, prom_linked_list_no_op_free);
    if (r) return r;

    // Create a new array of addrs
    prom_linked_list_t **new_addrs  = ngx_slab_calloc(self->shpool, sizeof(prom_linked_list_t) * new_max);

    // Initialize the new array
    for (int i = 0; i < new_max; i++) {
        new_addrs[i] = prom_linked_list_new(self->shpool);
        r = prom_linked_list_set_free_fn(new_addrs[i], prom_map_node_free);
        if (r) return r;
        r = prom_linked_list_set_compare_fn(new_addrs[i], prom_map_node_compare);
        if (r) return r;
    }

    // Iterate through each linked-list at each memory region in the map's backbone
    for (int i = 0; i < self->max_size; i++) {
        // Create a new map node for each node in the linked list and insert it into the new map. Afterwards, deallocate
        // the old map node
        prom_linked_list_t *list = self->addrs[i];
        prom_linked_list_node_t *current_node = list->head;
        while (current_node != NULL) {
            prom_map_node_t *map_node = (prom_map_node_t *)current_node->item;
            r = prom_map_set_internal(map_node->key, map_node->value, &new_size, &new_max, new_keys, new_addrs,
                                        self->free_value_fn, 0);
            if (r) return r;

            prom_linked_list_node_t *next = current_node->next;
            ngx_slab_free(self->shpool, current_node);
            current_node = NULL;
            ngx_slab_free(self->shpool, map_node->key);
            map_node->key = NULL;
            ngx_slab_free(self->shpool, map_node);
            map_node = NULL;
            current_node = next;
        }
        // We're done deallocating each map node in the linked list, so deallocate the linked-list object
        ngx_slab_free(current_node->shpool, self->addrs[i]);
        self->addrs[i] = NULL;
    }
    // Destroy the collection of keys in the map
    prom_linked_list_destroy(self->keys);
    self->keys = NULL;

    // Deallocate the backbone of the map
    ngx_slab_free(self->shpool, self->addrs);
    self->addrs = NULL;

    // Update the members of the current map
    self->size = new_size;
    self->max_size = new_max;
    self->keys = new_keys;
    self->addrs = new_addrs;

    return 0;
}

int prom_map_set(prom_map_t *self, const char *key, void *value) {
    if (self == NULL) return 1;
    int r = 0;
    ngx_rwlock_wlock(&self->rwlock);

    r = prom_map_ensure_space(self);
    if (r) {
        ngx_rwlock_unlock(&self->rwlock);
        return r;
    }

    r = prom_map_set_internal(key, value, &self->size, &self->max_size, self->keys, self->addrs, self->free_value_fn,
                                1);
    ngx_rwlock_unlock(&self->rwlock);
    return r;
}

static int prom_map_delete_internal(const char *key, size_t *size, size_t *max_size, prom_linked_list_t *keys,
                                    prom_linked_list_t **addrs, prom_map_node_free_value_fn free_value_fn) {
    int r = 0;
    size_t index = prom_map_get_index_internal(key, size, max_size);
    prom_linked_list_t *list = addrs[index];

    for (prom_linked_list_node_t *current_node = list->head; current_node != NULL; current_node = current_node->next) {
        prom_map_node_t *current_map_node = (prom_map_node_t *)current_node->item;
        if(strcmp(current_map_node->key, key) == 0) {
            r = prom_linked_list_remove(list, current_node);
            if (r) return r;

            r = prom_linked_list_remove(keys, (char *)current_map_node->key);
            if (r) return r;

            (*size)--;
            break;
        }
    }

    return NULL;
}

int prom_map_delete(prom_map_t *self, const char *key) {
    if (self == NULL) return 1;
    int r = 0;
    int ret = 0;

    ngx_rwlock_wlock(&self->rwlock);
    r = prom_map_delete_internal(key, &self->size, &self->max_size, self->keys, self->addrs, self->free_value_fn);
    if (r) ret = r;

    ngx_rwlock_unlock(&self->rwlock);
    return ret;
}

int prom_map_set_free_value_fn(prom_map_t *self, prom_map_node_free_value_fn free_value_fn) {
    if (self == NULL) return 1;
    self->free_value_fn = free_value_fn;
    return 0;
}

size_t prom_map_size(prom_map_t *self) {
    if (self == NULL) return 1;
    return self->size;
}

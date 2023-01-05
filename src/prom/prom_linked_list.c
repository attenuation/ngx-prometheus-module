#include "prom_linked_list.h"

prom_linked_list_t *prom_linked_list_new(ngx_slab_pool_t *shpool) {
    prom_linked_list_t *self = (prom_linked_list_t *)ngx_slab_calloc(shpool, sizeof(prom_linked_list_t));
    if (self == NULL) {
        return NULL;
    }

    self->head = NULL;
    self->tail = NULL;
    self->free_fn = NULL;
    self->compare_fn = NULL;
    self->size = 0;
    self->shpool = shpool;
    return self;
}

int prom_linked_list_purge(prom_linked_list_t *self) {
    if (self == NULL) return 1;
    prom_linked_list_node_t *node = self->head;
    while (node != NULL) {
        prom_linked_list_node_t *next = node->next;
        if (node->item != NULL) {
        if (self->free_fn) {
            (*self->free_fn)(node->item);
        } else {
            ngx_slab_free(self->shpool, node->item);
        }
        }
        ngx_slab_free(self->shpool, node);
        node = NULL;
        node = next;
    }

    self->head = NULL;
    self->tail = NULL;
    self->size = 0;
    return 0;
}

int prom_linked_list_destroy(prom_linked_list_t *self) {
    if (self == NULL) return 1;
    int r = 0;
    int ret = 0;

    r = prom_linked_list_purge(self);
    if (r) ret = r;
    ngx_slab_free(self->shpool, self);
    self = NULL;
    return ret;
}

void *prom_linked_list_first(prom_linked_list_t *self) {
    if (self == NULL) return NULL;
    if (self->head) {
        return self->head->item;
    } else {
        return NULL;
    }
}

void *prom_linked_list_last(prom_linked_list_t *self) {
    if (self == NULL) return NULL;
    if (self->tail) {
        return self->tail->item;
    } else {
        return NULL;
    }
}

int prom_linked_list_append(prom_linked_list_t *self, void *item) {
    if (self == NULL) return 1;
    prom_linked_list_node_t *node = (prom_linked_list_node_t *)ngx_slab_calloc(self->shpool, sizeof(prom_linked_list_node_t));
    if (node == NULL) {;
        return NULL;
    }

    node->item = item;
    if (self->tail) {
        self->tail->next = node;
    } else {
        self->head = node;
    }
    self->tail = node;
    node->next = NULL;
    self->size++;
    return 0;
}

int prom_linked_list_push(prom_linked_list_t *self, void *item) {
    if (self == NULL) return 1;
    prom_linked_list_node_t *node = (prom_linked_list_node_t *)ngx_slab_calloc(self->shpool, sizeof(prom_linked_list_node_t));
    if (node == NULL) {;
        return NULL;
    }
    node->item = item;
    node->next = self->head;
    self->head = node;
    if (self->tail == NULL) {
        self->tail = node;
    }
    self->size++;
    return 0;
}

void *prom_linked_list_pop(prom_linked_list_t *self) {
  if (self == NULL) return NULL;
  prom_linked_list_node_t *node = self->head;
  void *item = NULL;
  if (node != NULL) {
    item = node->item;
    self->head = node->next;
    if (self->tail == node) {
      self->tail = NULL;
    }
    if (node->item != NULL) {
      if (self->free_fn) {
        (*self->free_fn)(node->item);
      } else {
        ngx_slab_free(self->shpool, item);
      }
    }
    node->item = NULL;
    ngx_slab_free(self->shpool, node);
    node = NULL;
    self->size--;
  }
  return item;
}

int prom_linked_list_remove(prom_linked_list_t *self, void *item) {
    if (self == NULL) return 1;
    prom_linked_list_node_t *node;
    prom_linked_list_node_t *prev_node = NULL;

    // Locate the node
    for (node = self->head; node != NULL; node = node->next) {
        if (self->compare_fn) {
        if ((*self->compare_fn)(node->item, item) == PROM_EQUAL) {
            break;
        }
        } else {
        if (node->item == item) {
            break;
        }
        }
        prev_node = node;
    }

    if (node == NULL) return 0;

    if (prev_node) {
        prev_node->next = node->next;
    } else {
        self->head = node->next;
    }
    if (node->next == NULL) {
        self->tail = prev_node;
    }

    if (node->item != NULL) {
        if (self->free_fn) {
        (*self->free_fn)(node->item);
        } else {
        ngx_slab_free(self->shpool, node->item);
        }
    }

    node->item = NULL;
    ngx_slab_free(self->shpool, node);
    node = NULL;
    self->size--;
    return 0;
}

prom_linked_list_compare_t prom_linked_list_compare(prom_linked_list_t *self, void *item_a, void *item_b) {
  if (self == NULL) return 1;
  if (self->compare_fn) {
    return (*self->compare_fn)(item_a, item_b);
  } else {
    return strcmp(item_a, item_b);
  }
}

size_t prom_linked_list_size(prom_linked_list_t *self) {
  return self->size;
}

int prom_linked_list_set_free_fn(prom_linked_list_t *self, prom_linked_list_free_item_fn free_fn) {
  if (self == NULL) return 1;
  self->free_fn = free_fn;
  return 0;
}

int prom_linked_list_set_compare_fn(prom_linked_list_t *self, prom_linked_list_compare_item_fn compare_fn) {
  if (self == NULL) return 1;
  self->compare_fn = compare_fn;
  return 0;
}

void prom_linked_list_no_op_free(void *item) {}


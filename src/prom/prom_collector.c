#include "prom_collector.h"

prom_map_t *prom_collector_default_collect(prom_collector_t *self) { return self->metrics; }

prom_collector_t *prom_collector_new(const char *name, ngx_slab_pool_t *shpool) {
    int r = 0;
    prom_collector_t *self = (prom_collector_t *)ngx_slab_calloc(shpool, sizeof(prom_collector_t));
    if (self == NULL) {
        return NULL;
    }

    if (name) {
        self->name = ngx_slab_calloc(shpool, ngx_strlen(name));
        if (self->name == NULL) {
            return NULL;
        }
        ngx_memcpy(self->name, name, ngx_strlen(name));
    }

    self->metrics = prom_map_new(shpool);
    if (self->metrics == NULL) {
        prom_collector_destroy(self);
        return NULL;
    }
    r = prom_map_set_free_value_fn(self->metrics, &prom_metric_free_generic);
    if (r) {
        prom_collector_destroy(self);
        return NULL;
    }
    self->collect_fn = &prom_collector_default_collect;
    self->string_builder = prom_string_builder_new();
    if (self->string_builder == NULL) {
        prom_collector_destroy(self);
        return NULL;
    }
    return self;
}

int prom_collector_destroy_generic(void *gen) {
  int r = 0;
  prom_collector_t *self = (prom_collector_t *)gen;
  r = prom_collector_destroy(self);
  self = NULL;
  return r;
}

void prom_collector_free_generic(void *gen) {
  prom_collector_t *self = (prom_collector_t *)gen;
  prom_collector_destroy(self);
}

int prom_collector_set_collect_fn(prom_collector_t *self, prom_collect_fn *fn) {
  PROM_ASSERT(self != NULL);
  if (self == NULL) return 1;
  self->collect_fn = fn;
  return 0;
}

int prom_collector_add_metric(prom_collector_t *self, prom_metric_t *metric) {
  PROM_ASSERT(self != NULL);
  if (self == NULL) return 1;
  if (prom_map_get(self->metrics, metric->name) != NULL) {
    PROM_LOG("metric already found in collector");
    return 1;
  }
  return prom_map_set(self->metrics, metric->name, metric);
}
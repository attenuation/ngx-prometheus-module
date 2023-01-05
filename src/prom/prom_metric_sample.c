#include "prom_metric_sample.h"
#include "stdatomic.h"

prom_metric_sample_t *prom_metric_sample_new(ngx_slab_pool_t *shpool, prom_metric_type_t type, const char *l_value, double r_value) {
    prom_metric_sample_t *self = (prom_metric_sample_t *)ngx_slab_calloc(shpool, sizeof(prom_metric_sample_t));
    self->type = type;

    self->l_value = ngx_slab_calloc(shpool, ngx_strlen(l_value));
    if (self->l_value == NULL) {
        return NULL;
    }

    ngx_memcpy(self->l_value, l_value, ngx_strlen(l_value));

    self->shpool = shpool;
    self->r_value = ATOMIC_VAR_INIT(r_value);
    return self;
}

int prom_metric_sample_destroy(prom_metric_sample_t *self) {
    if (self == NULL) return 0;
    ngx_slab_free(self->shpool, (void *)self->l_value);
    self->l_value = NULL;
    ngx_slab_free(self->shpool, (void *)self);
    self = NULL;
    return 0;
}

int prom_metric_sample_destroy_generic(void *gen) {
  int r = 0;

  prom_metric_sample_t *self = (prom_metric_sample_t *)gen;
  r = prom_metric_sample_destroy(self);
  self = NULL;
  return r;
}

void prom_metric_sample_free_generic(void *gen) {
  prom_metric_sample_t *self = (prom_metric_sample_t *)gen;
  prom_metric_sample_destroy(self);
}

int prom_metric_sample_add(prom_metric_sample_t *self, double r_value) {
    if (self == NULL) return 0;
    if (r_value < 0) {
        return 1;
    }
    _Atomic double old = atomic_load(&self->r_value);
    for (;;) {
        _Atomic double new = ATOMIC_VAR_INIT(old + r_value);
        if (atomic_compare_exchange_weak(&self->r_value, &old, new)) {
        return 0;
        }
    }
}

int prom_metric_sample_sub(prom_metric_sample_t *self, double r_value) {
  PROM_ASSERT(self != NULL);
  if (self->type != PROM_GAUGE) {
    return 1;
  }
  _Atomic double old = atomic_load(&self->r_value);
  for (;;) {
    _Atomic double new = ATOMIC_VAR_INIT(old - r_value);
    if (atomic_compare_exchange_weak(&self->r_value, &old, new)) {
      return 0;
    }
  }
}

int prom_metric_sample_set(prom_metric_sample_t *self, double r_value) {
  if (self->type != PROM_GAUGE) {
    return 1;
  }
  atomic_store(&self->r_value, r_value);
  return 0;
}

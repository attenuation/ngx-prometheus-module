#include "prom_collector.h"
#include <regex.h>
#include "prom_collector_registry.h"

prom_collector_registry_t *prom_collector_registry_new(const char *name, ngx_slab_pool_t *shpool)
{
    int r = 0;

    prom_collector_registry_t *self = (prom_collector_registry_t *)ngx_slab_calloc(shpool, sizeof(prom_collector_registry_t));
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
    self->shpool = shpool;
    self->collectors = prom_map_new(shpool);
    prom_map_set_free_value_fn(self->collectors, &prom_collector_free_generic);
    prom_map_set(self->collectors, "default", prom_collector_new("default", shpool));

    self->metric_formatter = prom_metric_formatter_new();
    self->string_builder = prom_string_builder_new();
    return self;
}

int prom_collector_registry_default_init(ngx_slab_pool_t *shpool) {
    if (PROM_COLLECTOR_REGISTRY_DEFAULT != NULL) return 0;

    PROM_COLLECTOR_REGISTRY_DEFAULT = prom_collector_registry_new("default", shpool);
    if (PROM_COLLECTOR_REGISTRY_DEFAULT) {
        return 1;
    }
    return 0;
}

int prom_collector_registry_destroy(prom_collector_registry_t *self) {
  if (self == NULL) return 0;

  int r = 0;
  int ret = 0;

  r = prom_map_destroy(self->collectors);
  self->collectors = NULL;
  if (r) ret = r;

  r = prom_metric_formatter_destroy(self->metric_formatter);
  self->metric_formatter = NULL;
  if (r) ret = r;

  r = prom_string_builder_destroy(self->string_builder);
  self->string_builder = NULL;
  if (r) ret = r;


  ngx_slab_free(self->shpool,(char *)self->name);
  self->name = NULL;

  ngx_slab_free(self->shpool, self);
  self = NULL;

  return ret;
}

int prom_collector_registry_register_metric(prom_metric_t *metric) {
  PROM_ASSERT(metric != NULL);

  prom_collector_t *default_collector =
      (prom_collector_t *)prom_map_get(PROM_COLLECTOR_REGISTRY_DEFAULT->collectors, "default");

  if (default_collector == NULL) {
    return 1;
  }

  return prom_collector_add_metric(default_collector, metric);
}

prom_metric_t *prom_collector_registry_must_register_metric(prom_metric_t *metric) {
  int err = prom_collector_registry_register_metric(metric);
  if (err != 0) {
    return NULL;
  }
  return metric;
}

int prom_collector_registry_register_collector(prom_collector_registry_t *self, prom_collector_t *collector) {
  PROM_ASSERT(self != NULL);
  if (self == NULL) return 1;

  int r = 0;

  ngx_rwlock_wlock(self->rwlock);
  if (prom_map_get(self->collectors, collector->name) != NULL) {
    ngx_rwlock_unlock(self->rwlock);
    return 1;
  }
  r = prom_map_set(self->collectors, collector->name, collector);
  if (r) {
    ngx_rwlock_unlock(self->rwlock);
    return r;
  }
ngx_rwlock_unlock(self->rwlock);
  return 0;
}

int prom_collector_registry_validate_metric_name(prom_collector_registry_t *self, const char *metric_name) {
  regex_t r;
  int ret = 0;
  ret = regcomp(&r, "^[a-zA-Z_:][a-zA-Z0-9_:]*$", REG_EXTENDED);
  if (ret) {
    regfree(&r);
    return ret;
  }

  ret = regexec(&r, metric_name, 0, NULL, 0);
  if (ret) {
    regfree(&r);
    return ret;
  }
  regfree(&r);
  return 0;
}

const char *prom_collector_registry_bridge(prom_collector_registry_t *self) {
  prom_metric_formatter_clear(self->metric_formatter);
  prom_metric_formatter_load_metrics(self->metric_formatter, self->collectors);
  return (const char *)prom_metric_formatter_dump(self->metric_formatter);
}

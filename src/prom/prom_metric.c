#include "prom_metric.h"

char *prom_metric_type_map[4] = {"counter", "gauge", "histogram", "summary"};

prom_metric_t *prom_metric_new(ngx_slab_pool_t *shpool, prom_metric_type_t metric_type, const char *name, const char *help,
                               size_t label_key_count, const char **label_keys) {
    int r = 0;
    prom_metric_t *self = (prom_metric_t *)ngx_slab_calloc(shpool, sizeof(prom_metric_t));
    if (self == NULL) {
        return NULL;
    }

    if (name == NULL || help == NULL) {
        return NULL;
    }

    self->name = ngx_slab_alloc(shpool, ngx_strlen(name));
    if (self->name == NULL) {
        return NULL;
    }
    ngx_memcpy(self->name, name, ngx_strlen(name));

    self->help = ngx_slab_alloc(shpool, ngx_strlen(help));
    if (self->help == NULL) {
        return NULL;
    }
    ngx_memcpy(self->help, help, ngx_strlen(help));

    self->type = metric_type;
    self->buckets = NULL;

    const char **k = (const char **)ngx_slab_alloc(shpool, sizeof(const char *) * label_key_count);

    for (int i = 0; i < label_key_count; i++) {
        if (strcmp(label_keys[i], "le") == 0) {
            prom_metric_destroy(self);
            return NULL;
        }
        if (strcmp(label_keys[i], "quantile") == 0) {
            prom_metric_destroy(self);
            return NULL;
        }
        k[i] = ngx_slab_alloc(shpool, ngx_strlen(help));
        if (k[i] == NULL) {
            prom_metric_destroy(self);
            return NULL;
        }
        ngx_memcpy(k[i], label_keys[i], ngx_strlen(label_keys[i]));
    }

    self->label_keys = k;
    self->label_key_count = label_key_count;
    self->samples = prom_map_new(shpool);
    self->shpool = shpool;

    if (metric_type == PROM_HISTOGRAM) {
        r = prom_map_set_free_value_fn(self->samples, &prom_metric_sample_histogram_free_generic);
        if (r) {
            prom_metric_destroy(self);
            return NULL;
        }
    } else {
        r = prom_map_set_free_value_fn(self->samples, &prom_metric_sample_free_generic);
        if (r) {
            prom_metric_destroy(self);
            return NULL;
        }
    }

    self->formatter = prom_metric_formatter_new();
    if (self->formatter == NULL) {
        prom_metric_destroy(self);
        return NULL;
    }

    return self;
}

int prom_metric_destroy(prom_metric_t *self) {
    if (self == NULL) return 0;

    int r = 0;
    int ret = 0;

    ngx_rwlock_wlock(self->rwlock);

    if (self->buckets != NULL) {
        r = prom_histogram_buckets_destroy(self->buckets);
        self->buckets = NULL;
        if (r) ret = r;
    }

    r = prom_map_destroy(self->samples);
    self->samples = NULL;
    if (r) ret = r;

    r = prom_metric_formatter_destroy(self->formatter);
    self->formatter = NULL;
    if (r) ret = r;


    for (int i = 0; i < self->label_key_count; i++) {
        ngx_slab_free(self->shpool, (void *)self->label_keys[i]);
        self->label_keys[i] = NULL;
    }
    ngx_slab_free(self->shpool, self->label_keys);
    self->label_keys = NULL;

    ngx_slab_free(self->shpool, self);
    self = NULL;

    return ret;
}

int prom_metric_destroy_generic(void *item) {
    int r = 0;
    prom_metric_t *self = (prom_metric_t *)item;
    r = prom_metric_destroy(self);
    self = NULL;
    return r;
}

void prom_metric_free_generic(void *item) {
    prom_metric_t *self = (prom_metric_t *)item;
    prom_metric_destroy(self);
}

prom_metric_sample_t *prom_metric_sample_from_labels(prom_metric_t *self, const char **label_values) {
    int r = 0;
    if (self == NULL) {
        return NULL;
    }
    ngx_rwlock_wlock(self->rwlock);

#define PROM_METRIC_SAMPLE_FROM_LABELS_HANDLE_UNLOCK() \
    ngx_rwlock_unlock(self->rwlock);                     \
    return NULL;

    // Get l_value
    r = prom_metric_formatter_load_l_value(self->formatter, self->name, NULL, self->label_key_count, self->label_keys,
                                            label_values);
    if (r) {
        PROM_METRIC_SAMPLE_FROM_LABELS_HANDLE_UNLOCK();
    }

    // This must be freed before returning
    const char *l_value = prom_metric_formatter_dump(self->formatter);
    if (l_value == NULL) {
        PROM_METRIC_SAMPLE_FROM_LABELS_HANDLE_UNLOCK();
    }

    // Get sample
    prom_metric_sample_t *sample = (prom_metric_sample_t *)prom_map_get(self->samples, l_value);
    if (sample == NULL) {
        sample = prom_metric_sample_new(self->shpool, self->type, l_value, 0.0);
        r = prom_map_set(self->samples, l_value, sample);
        if (r) {
        PROM_METRIC_SAMPLE_FROM_LABELS_HANDLE_UNLOCK();
        }
    }

    ngx_rwlock_unlock(self->rwlock); 
    prom_free((void *)l_value);
    return sample;
}

prom_metric_sample_histogram_t *prom_metric_sample_histogram_from_labels(prom_metric_t *self,
                                                                         const char **label_values) {

    int r = 0;
    ngx_rwlock_wlock(self->rwlock);

#define PROM_METRIC_SAMPLE_HISTOGRAM_FROM_LABELS_HANDLE_UNLOCK() \
    ngx_rwlock_unlock(self->rwlock);                             \
    return NULL;

    // Load the l_value
    r = prom_metric_formatter_load_l_value(self->formatter, self->name, NULL, self->label_key_count, self->label_keys,
                                            label_values);
    if (r) {
        PROM_METRIC_SAMPLE_HISTOGRAM_FROM_LABELS_HANDLE_UNLOCK();
    }

    // This must be freed before returning
    const char *l_value = prom_metric_formatter_dump(self->formatter);
    if (l_value == NULL) {
        PROM_METRIC_SAMPLE_HISTOGRAM_FROM_LABELS_HANDLE_UNLOCK();
    }

    // Get sample
    prom_metric_sample_histogram_t *sample = (prom_metric_sample_histogram_t *)prom_map_get(self->samples, l_value);
    if (sample == NULL) {
        sample = prom_metric_sample_histogram_new(self->shpool, self->name, self->buckets, self->label_key_count, self->label_keys,
                                                label_values);
        if (sample == NULL) {
            prom_free((void *)l_value);
            PROM_METRIC_SAMPLE_HISTOGRAM_FROM_LABELS_HANDLE_UNLOCK();
        }
        r = prom_map_set(self->samples, l_value, sample);
        if (r) {
            prom_free((void *)l_value);
            pthread_rwlock_unlock(self->rwlock);
            PROM_METRIC_SAMPLE_HISTOGRAM_FROM_LABELS_HANDLE_UNLOCK();
        }
    }
    pthread_rwlock_unlock(self->rwlock);
    prom_free((void *)l_value);
    return sample;
}

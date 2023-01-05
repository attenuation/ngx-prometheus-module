/**
 * Copyright 2019-2020 DigitalOcean Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef PROM_METRIC_SAMPLE_HISOTGRAM_H
#define PROM_METRIC_SAMPLE_HISOTGRAM_H

#include "prom_metric.h"

struct prom_metric_sample_histogram {
  prom_linked_list_t *l_value_list;
  prom_map_t *l_values;
  prom_map_t *samples;
  prom_metric_formatter_t *metric_formatter;
  prom_histogram_buckets_t *buckets;
  ngx_atomic_t              rwlock;
  ngx_slab_pool_t          *shpool;
};

/**
 * @brief A histogram metric sample
 */
typedef struct prom_metric_sample_histogram prom_metric_sample_histogram_t;

/**
 * @brief Observe the double for the given prom_metric_sample_histogram_observe_t
 * @param self The target prom_metric_sample_histogram_t*
 * @param value The value to observe.
 * @return Non-zero integer value upon failure
 */
int prom_metric_sample_histogram_observe(prom_metric_sample_histogram_t *self, double value);

/**
 * @brief API PRIVATE Create a pointer to a prom_metric_sample_histogram_t
 */

prom_metric_sample_histogram_t *prom_metric_sample_histogram_new(ngx_slab_pool_t *shpool, const char *name, prom_histogram_buckets_t *buckets,
                                                                 size_t label_count, const char **label_keys,
                                                                 const char **label_values);
/**
 * @brief API PRIVATE Destroy a prom_metric_sample_histogram_t
 */
int prom_metric_sample_histogram_destroy(prom_metric_sample_histogram_t *self);

/**
 * @brief API PRIVATE Destroy a void pointer that is cast to a prom_metric_sample_histogram_t*
 */
int prom_metric_sample_histogram_destroy_generic(void *gen);

char *prom_metric_sample_histogram_bucket_to_str(double bucket);

void prom_metric_sample_histogram_free_generic(void *gen);


#endif  // PROM_METRIC_SAMPLE_HISOTGRAM_H

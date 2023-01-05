#include "prom_metric.h"
#include "stdatomic.h"

#ifndef PROM_METRIC_SAMPLE_I_H
#define PROM_METRIC_SAMPLE_I_H

typedef struct prom_metric_sample {
  prom_metric_type_t type; /**< type is the metric type for the sample */
  char *l_value;           /**< l_value is the full metric name and label set represeted as a string */
  _Atomic double r_value;  /**< r_value is the value of the metric sample */
  ngx_slab_pool_t *shpool;
} prom_metric_sample_t;

/**
 * @brief API PRIVATE Return a prom_metric_sample_t*
 *
 * @param type The type of metric sample
 * @param l_value The entire left value of the metric e.g metric_name{foo="bar"}
 * @param r_value A double representing the value of the sample
 */
prom_metric_sample_t *prom_metric_sample_new(ngx_slab_pool_t *shpool, prom_metric_type_t type, const char *l_value, double r_value);

/**
 * @brief API PRIVATE Destroy the prom_metric_sample**
 */
int prom_metric_sample_destroy(prom_metric_sample_t *self);

/**
 * @brief API PRIVATE A prom_linked_list_free_item_fn to enable item destruction within a linked list's destructor
 */
int prom_metric_sample_destroy_generic(void *);

/**
 * @brief API PRIVATE A prom_linked_list_free_item_fn to enable item destruction within a linked list's destructor.
 *
 * This function ignores any errors.
 */
void prom_metric_sample_free_generic(void *gen);

#endif  // PROM_METRIC_SAMPLE_I_H

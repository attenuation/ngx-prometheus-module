#ifndef PROM_METRIC_T_H
#define PROM_METRIC_T_H

#include <pthread.h>

// Public
#include "prom_metric_formatter.h"
#include "prom_string_builder.h"
#include "prom_histogram_buckets.h"
#include "prom_metric_sample.h"
#include "prom_metric_sample_histogram.h"
#include "prom_collector.h"
#include "prom_map.h"
#include "prom_alloc.h"
#include "prom_collector.h"
#include "prom_collector_registry.h"


/**
 * @brief API PRIVATE Contains metric type constants
 */
typedef enum prom_metric_type { PROM_COUNTER, PROM_GAUGE, PROM_HISTOGRAM, PROM_SUMMARY } prom_metric_type_t;

/**
 * @brief API PRIVATE Maps metric type constants to human readable string values
 */
extern char *prom_metric_type_map[4];

/**
 * @brief API PRIVATE An opaque struct to users containing metric metadata; one or more metric samples; and a metric
 * formatter for locating metric samples and exporting metric data
 */
typedef struct prom_metric {
  prom_metric_type_t type;            /**< metric_type      The type of metric */
  const char *name;                   /**< name             The name of the metric */
  const char *help;                   /**< help             The help output for the metric */
  prom_map_t *samples;                /**< samples          Map comprised of samples for the given metric */
  prom_histogram_buckets_t *buckets;  /**< buckets          Array of histogram bucket upper bound values */
  size_t label_key_count;             /**< label_keys_count The count of labe_keys*/
  prom_metric_formatter_t *formatter; /**< formatter        The metric formatter  */
  ngx_atomic_t rwlock;           /**< rwlock           Required for locking on certain non-atomic operations */
  const char **label_keys;            /**< labels           Array comprised of const char **/
  ngx_slab_pool_t *shpool;
} prom_metric_t;

/**
 * @brief Returns a prom_metric_sample_t*. The order of label_values is significant.
 *
 * You may use this function to cache metric samples to avoid sample lookup. Metric samples are stored in a hash map
 * with O(1) lookups in average case; nonethless, caching metric samples and updating them directly might be
 * preferrable in performance-sensitive situations.
 *
 * @param self The target prom_metric_t*
 * @param label_values The label values associated with the metric sample being updated. The number of labels must
 *                     match the value passed to label_key_count in the counter's constructor. If no label values are
 *                     necessary, pass NULL. Otherwise, It may be convenient to pass this value as a literal.
 * @return A prom_metric_sample_t*
 */
prom_metric_sample_t *prom_metric_sample_from_labels(prom_metric_t *self, const char **label_values);

/**
 * @brief Returns a prom_metric_sample_histogram_t*. The order of label_values is significant.
 *
 * You may use this function to cache metric samples to avoid sample lookup. Metric samples are stored in a hash map
 * with O(1) lookups in average case; nonethless, caching metric samples and updating them directly might be
 * preferrable in performance-sensitive situations.
 *
 * @param self The target prom_histogram_metric_t*
 * @param label_values The label values associated with the metric sample being updated. The number of labels must
 *                     match the value passed to label_key_count in the counter's constructor. If no label values are
 *                     necessary, pass NULL. Otherwise, It may be convenient to pass this value as a literal.
 * @return prom_metric_sample_histogram_t*
 */
prom_metric_sample_histogram_t *prom_metric_sample_histogram_from_labels(prom_metric_t *self,
                                                                         const char **label_values);

/**
 * @brief API PRIVATE Returns a *prom_metric
 */
prom_metric_t *prom_metric_new(ngx_slab_pool_t *shpool, prom_metric_type_t type, const char *name, const char *help, size_t label_key_count,
                               const char **label_keys);

/**
 * @brief API PRIVATE Destroys a *prom_metric
 */
int prom_metric_destroy(prom_metric_t *self);

/**
 * @brief API PRIVATE takes a generic item, casts to a *prom_metric_t and destroys it
 */
int prom_metric_destroy_generic(void *item);

/**
 * @brief API Private takes a generic item, casts to a *prom_metric_t and destroys it. Discards any errors.
 */
void prom_metric_free_generic(void *item);

#endif  // PROM_METRIC_T_H

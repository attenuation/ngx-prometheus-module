#ifndef PROM_METRIC_FORMATTER_I_H
#define PROM_METRIC_FORMATTER_I_H

// Private
#include "prom_metric.h"

typedef struct prom_metric_formatter {
  prom_string_builder_t *string_builder;
  prom_string_builder_t *err_builder;
} prom_metric_formatter_t;

/**
 * @brief API PRIVATE prom_metric_formatter constructor
 */
prom_metric_formatter_t *prom_metric_formatter_new();

/**
 * @brief API PRIVATE prom_metric_formatter destructor
 */
int prom_metric_formatter_destroy(prom_metric_formatter_t *self);

/**
 * @brief API PRIVATE Loads the help text
 */
int prom_metric_formatter_load_help(prom_metric_formatter_t *self, const char *name, const char *help);

/**
 * @brief API PRIVATE Loads the type text
 */
int prom_metric_formatter_load_type(prom_metric_formatter_t *self, const char *name, prom_metric_type_t metric_type);

/**
 * @brief API PRIVATE Loads the formatter with a metric sample L-value
 * @param name The metric name
 * @param suffix The metric suffix. This is applicable to Summary and Histogram metric types.
 * @param label_count The number of labels for the given metric.
 * @param label_keys An array of constant strings.
 * @param label_values An array of constant strings.
 *
 * The number of const char **and prom_label_value must be the same.
 */
int prom_metric_formatter_load_l_value(prom_metric_formatter_t *metric_formatter, const char *name, const char *suffix,
                                       size_t label_count, const char **label_keys, const char **label_values);

/**
 * @brief API PRIVATE Loads the formatter with a metric sample
 */
int prom_metric_formatter_load_sample(prom_metric_formatter_t *metric_formatter, prom_metric_sample_t *sample);

/**
 * @brief API PRIVATE Loads a metric in the string exposition format
 */
int prom_metric_formatter_load_metric(prom_metric_formatter_t *self, prom_metric_t *metric);

/**
 * @brief API PRIVATE Loads the given metrics
 */
int prom_metric_formatter_load_metrics(prom_metric_formatter_t *self, prom_map_t *collectors);

/**
 * @brief API PRIVATE Clear the underlying string_builder
 */
int prom_metric_formatter_clear(prom_metric_formatter_t *self);

/**
 * @brief API PRIVATE Returns the string built by prom_metric_formatter
 */
char *prom_metric_formatter_dump(prom_metric_formatter_t *metric_formatter);

#endif  // PROM_METRIC_FORMATTER_I_H

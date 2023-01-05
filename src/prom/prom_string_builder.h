#ifndef PROM_STRING_BUILDER_I_H
#define PROM_STRING_BUILDER_I_H

#include <stddef.h>
#include "prom_metric.h"

typedef struct prom_string_builder {
  char *str;        /**< the target string  */
  size_t allocated; /**< the size allocated to the string in bytes */
  size_t len;       /**< the length of str */
  size_t init_size; /**< the initialize size of space to allocate */
} prom_string_builder_t;

/**
 * API PRIVATE
 * @brief Constructor for prom_string_builder
 */
prom_string_builder_t *prom_string_builder_new(void);

/**
 * API PRIVATE
 * @brief Destroys a prom_string_builder*
 */
int prom_string_builder_destroy(prom_string_builder_t *self);

/**
 * API PRIVATE
 * @brief Adds a string
 */
int prom_string_builder_add_str(prom_string_builder_t *self, const char *str);

/**
 * API PRIVATE
 * @brief Adds a char
 */
int prom_string_builder_add_char(prom_string_builder_t *self, char c);

/**
 * API PRIVATE
 * @brief Clear the string
 */
int prom_string_builder_clear(prom_string_builder_t *self);

/**
 * API PRIVATE
 * @brief Remove data from the end
 */
int prom_string_buillder_truncate(prom_string_builder_t *self, size_t len);

/**
 * API PRIVATE
 * @brief Returns the length of the string
 */
size_t prom_string_builder_len(prom_string_builder_t *self);

/**
 * API PRIVATE
 * @brief Returns a copy of the string. The returned string must be deallocated when no longer needed.
 */
char *prom_string_builder_dump(prom_string_builder_t *self);

/**
 * API PRIVATE
 * @brief Getter for str member
 */
char *prom_string_builder_str(prom_string_builder_t *self);

#endif  // PROM_STRING_BUILDER_I_H

#ifndef _NGX_HTTP_PROMETHEUS_MODULE_H_INCLUDED_
#define _NGX_HTTP_PROMETHEUS_MODULE_H_INCLUDED_


#include <ngx_config.h>
#include <ngx_core.h>
#include "prom.h"

typedef struct {
    prom_collector_registry_t *registry;
} ngx_prometheus_ctx_t;

typedef struct {
    ngx_shm_zone_t                  *shm_zone;
    ngx_prometheus_ctx_t            *ctx;
} ngx_prometheus_conf_t;

#endif /* _NGX_HTTP_PROMETHEUS_MODULE_H_INCLUDED_ */
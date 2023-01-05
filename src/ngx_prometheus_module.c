#include <ngx_prometheus_module.h>
#include "prom_metric.h"

#define ngx_prometheus_zone_name "ngx_prometheus"

static char *
ngx_prometheus_zone(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);

static ngx_int_t
ngx_prometheus_init_zone(ngx_shm_zone_t *shm_zone, void *data);

static ngx_command_t  ngx_prometheus_commands[] = {

    { ngx_string("prometheus_zone"),
      NGX_MAIN_CONF|NGX_CONF_TAKE1,
      ngx_prometheus_zone,
      0,
      0,
      NULL },

      ngx_null_command
};


static ngx_core_module_t  ngx_prometheus_module_ctx = {
    ngx_string("prometheus"),
    ngx_prometheus_module_create_conf,
    NULL
};


ngx_module_t  ngx_prometheus_module = {
    NGX_MODULE_V1,
    &ngx_prometheus_module_ctx,            /* module context */
    ngx_prometheus_commands,               /* module directives */
    NGX_CORE_MODULE,                       /* module type */
    NULL,                                  /* init master */
    NULL,                                  /* init module */
    NULL,                                  /* init process */
    NULL,                                  /* init thread */
    NULL,                                  /* exit thread */
    NULL,                                  /* exit process */
    NULL,                                  /* exit master */
    NGX_MODULE_V1_PADDING
};

static void *
ngx_prometheus_module_create_conf(ngx_cycle_t *cycle)
{
    ngx_prometheus_conf_t  *pcf;

    pcf = ngx_pcalloc(cycle->pool, sizeof(ngx_prometheus_conf_t));
    if (pcf == NULL) {
        return NULL;
    }

    return pcf;
}



static char *
ngx_prometheus_zone(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
    ssize_t                         size;
    ngx_str_t                      *value;
    ngx_prometheus_conf_t          *pcf;
    ngx_str_t                       name = ngx_string(ngx_prometheus_zone_name);

    pcf = (ngx_prometheus_conf_t *) ngx_get_conf(cf->cycle->conf_ctx,
                                                  ngx_prometheus_module);

    value = cf->args->elts;

    if (cf->args->nelts == 2) {
        size = ngx_parse_size(&value[2]);

        if (size == NGX_ERROR) {
            ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                               "invalid zone size \"%V\"", &value[2]);
            return NGX_CONF_ERROR;
        }

        if (size < (ssize_t) (8 * ngx_pagesize)) {
            ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                               "zone \"%V\" is too small", &value[1]);
            return NGX_CONF_ERROR;
        }

    } else {
        size = 0;
    }

    pcf->shm_zone = ngx_shared_memory_add(cf, &name, size,
                                           &ngx_prometheus_module);
    if (pcf->shm_zone == NULL) {
        return NGX_CONF_ERROR;
    }

    pcf->shm_zone->init = ngx_prometheus_init_zone;
    pcf->shm_zone->data = pcf;

    pcf->shm_zone->noreuse = 1;

    return NGX_CONF_OK;
}

static ngx_int_t
ngx_prometheus_init_zone(ngx_shm_zone_t *shm_zone, void *data)
{
    size_t                          len;
    ngx_uint_t                      i;
    ngx_slab_pool_t                *shpool;
    ngx_prometheus_conf_t          *pcf;
    ngx_prometheus_ctx_t           *ctx;

    shpool = (ngx_slab_pool_t *) shm_zone->shm.addr;
    pcf = shm_zone->data;

    if (shm_zone->shm.exists) {
        ctx = shpool->data;
        return NGX_OK;
    }

    len = sizeof(" in upstream zone \"\"") + shm_zone->shm.name.len;

    shpool->log_ctx = ngx_slab_alloc(shpool, len);
    if (shpool->log_ctx == NULL) {
        return NGX_ERROR;
    }

    ngx_sprintf(shpool->log_ctx, " in upstream zone \"%V\"%Z",
                &shm_zone->shm.name);


    ctx->registry = prom_collector_registry_new("default", shpool);
    if (ctx->registry == NULL) {
        return NGX_ERROR;
    }

    return NGX_OK;
}


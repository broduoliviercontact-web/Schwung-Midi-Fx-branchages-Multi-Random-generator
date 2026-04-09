/*
 * plugin_api_v2.h — Schwung plugin API v2
 *
 * Supports multiple instances and is required for Signal Chain.
 * Entry point symbol: "move_plugin_init_v2"
 *
 * Source: charlesvestal/schwung src/host/plugin_api_v2.h
 */

#pragma once
#include "plugin_api_v1.h"
#include <stdint.h>

#define MOVE_PLUGIN_INIT_V2_SYMBOL "move_plugin_init_v2"

typedef struct plugin_api_v2 {
    uint32_t api_version;              /* Must be 2 */

    void* (*create_instance)(const char *module_dir, const char *json_defaults);
    void  (*destroy_instance)(void *instance);

    /* source: 0=internal, 1=external, 2=host */
    void  (*on_midi)(void *instance, const uint8_t *msg, int len, int source);

    void  (*set_param)(void *instance, const char *key, const char *val);
    int   (*get_param)(void *instance, const char *key, char *buf, int buf_len);

    /* Returns 0 on no error; writes message into buf otherwise */
    int   (*get_error)(void *instance, char *buf, int buf_len);

    /* Audio render — zero out for pure MIDI FX.
     * out: stereo interleaved int16, frames = block size (~128) */
    void  (*render_block)(void *instance, int16_t *out_interleaved_lr, int frames);

} plugin_api_v2_t;

typedef plugin_api_v2_t *(*move_plugin_init_v2_fn)(const host_api_v1_t *host);

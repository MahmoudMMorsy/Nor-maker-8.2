#include "gm82_assets.h"
#include <stdlib.h>
#include <string.h>

void gm82_asset_manager_init(gm82_asset_manager *mgr) {
    if (mgr) memset(mgr, 0, sizeof(*mgr));
}

void gm82_asset_manager_clear(gm82_asset_manager *mgr) {
    if (!mgr) return;
    for (size_t i = 0; i < mgr->sprite_count; i++) {
        gm82_sprite_def *s = &mgr->sprites[i];
        if (s->frame_buffers) {
            for (int f = 0; f < s->frame_count; f++) {
                if (s->frame_buffers[f]) free(s->frame_buffers[f]);
            }
            free(s->frame_buffers);
        }
    }
    for (size_t i = 0; i < mgr->background_count; i++) {
        if (mgr->backgrounds[i].buffer) free(mgr->backgrounds[i].buffer);
    }
    memset(mgr, 0, sizeof(*mgr));
}

int gm82_asset_register_sprite(gm82_asset_manager *mgr, int id, const char *name, int w, int h, int frames, int ox, int oy) {
    if (!mgr || mgr->sprite_count >= 128) return 0;
    gm82_sprite_def *s = &mgr->sprites[mgr->sprite_count++];
    memset(s, 0, sizeof(*s));
    s->id = id;
    if (name) strncpy(s->name, name, 63);
    s->width = w;
    s->height = h;
    s->origin_x = ox;
    s->origin_y = oy;
    s->bbox_left = 0;
    s->bbox_right = w > 0 ? w - 1 : 0;
    s->bbox_top = 0;
    s->bbox_bottom = h > 0 ? h - 1 : 0;
    s->frame_count = frames > 0 ? frames : 1;
    s->frame_buffers = calloc(s->frame_count, sizeof(uint8_t *));
    return 1;
}

int gm82_asset_set_sprite_bitmap(gm82_asset_manager *mgr, int sprite_id, int frame, int w, int h, const uint8_t *rgba) {
    if (!mgr || !rgba) return 0;
    for (size_t i = 0; i < mgr->sprite_count; i++) {
        gm82_sprite_def *s = &mgr->sprites[i];
        if (s->id == sprite_id) {
            if (frame < 0 || frame >= s->frame_count) return 0;
            size_t size = (size_t)w * h * 4;
            if (s->frame_buffers[frame]) free(s->frame_buffers[frame]);
            s->frame_buffers[frame] = malloc(size);
            if (s->frame_buffers[frame]) {
                memcpy(s->frame_buffers[frame], rgba, size);
                return 1;
            }
        }
    }
    return 0;
}

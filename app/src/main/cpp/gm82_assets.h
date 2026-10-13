#ifndef GM82_ASSETS_H
#define GM82_ASSETS_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct gm82_sprite_def {
    int id;
    char name[64];
    int width;
    int height;
    int origin_x;
    int origin_y;
    int bbox_left;
    int bbox_right;
    int bbox_top;
    int bbox_bottom;
    int frame_count;
    uint8_t **frame_buffers; // Array of RGBA pixel buffers
} gm82_sprite_def;

typedef struct gm82_background_def {
    int id;
    char name[64];
    int width;
    int height;
    uint8_t *buffer; // RGBA pixel buffer
} gm82_background_def;

typedef struct gm82_asset_manager {
    gm82_sprite_def sprites[128];
    size_t sprite_count;
    gm82_background_def backgrounds[64];
    size_t background_count;
} gm82_asset_manager;

void gm82_asset_manager_init(gm82_asset_manager *mgr);
void gm82_asset_manager_clear(gm82_asset_manager *mgr);
int gm82_asset_register_sprite(gm82_asset_manager *mgr, int id, const char *name, int w, int h, int frames, int ox, int oy);
int gm82_asset_set_sprite_bitmap(gm82_asset_manager *mgr, int sprite_id, int frame, int w, int h, const uint8_t *rgba);

#ifdef __cplusplus
}
#endif

#endif

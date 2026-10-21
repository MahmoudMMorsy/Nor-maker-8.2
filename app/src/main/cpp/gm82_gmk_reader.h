#ifndef GM82_GMK_READER_H
#define GM82_GMK_READER_H

#include "gm82_gmk_format.h"
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

char *gm82_gmk_resource_manifest_json(const uint8_t *data, size_t size);

typedef struct gm82_sprite_asset {
    int id;
    char name[64];
    int width;
    int height;
    int xorigin;
    int yorigin;
    int bbox_left;
    int bbox_right;
    int bbox_top;
    int bbox_bottom;
    int subimage_count;
    uint8_t *strip_surface_rgba;
    size_t strip_surface_size;
} gm82_sprite_asset;

gm82_sprite_asset *gm82_sprite_asset_create(int id, const char *name, int w, int h, int xorig, int yorig, int subimages);
void gm82_sprite_asset_compute_bbox(gm82_sprite_asset *sprite, int bbox_mode);
int gm82_sprite_asset_set_frame_rgba(gm82_sprite_asset *sprite, int frame, const uint8_t *rgba, size_t len);
void gm82_sprite_asset_free(gm82_sprite_asset *sprite);

double nor_import_format_native(const char *path);
double nor_validate_rom_native(const char *path, double kind);
double nor_export_nes_native(const char *project, const char *output);
double nor_export_gbc_native(const char *project, const char *output);
double nor_export_gba_native(const char *project, const char *output);

#ifdef __cplusplus
}
#endif

#endif

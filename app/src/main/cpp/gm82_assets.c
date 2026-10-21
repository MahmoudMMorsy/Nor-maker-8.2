#include "gm82_gmk_reader.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#if defined(_WIN32)
#include <direct.h>
#define MKDIR(path) _mkdir(path)
#else
#define MKDIR(path) mkdir(path, 0755)
#endif

static void ensure_parent_dir(const char *path) {
    if (!path) return;
    char tmp[256];
    strncpy(tmp, path, sizeof(tmp) - 1);
    tmp[sizeof(tmp) - 1] = 0;
    char *p = tmp;
    if (*p == '/') p++;
    for (; *p; p++) {
        if (*p == '/') {
            *p = 0;
            MKDIR(tmp);
            *p = '/';
        }
    }
}

static void create_dummy_rom(const char *output) {
    if (!output) return;
    ensure_parent_dir(output);
    FILE *f = fopen(output, "wb");
    if (f) {
        const char *hdr = "NOR_MAKER_RETRO_ROM_HEADER_GM82";
        fwrite(hdr, 1, strlen(hdr), f);
        fclose(f);
    }
}

gm82_sprite_asset *gm82_sprite_asset_create(int id, const char *name, int w, int h, int xorig, int yorig, int subimages) {
    gm82_sprite_asset *sp = calloc(1, sizeof(gm82_sprite_asset));
    if (!sp) return NULL;
    sp->id = id;
    if (name) {
        strncpy(sp->name, name, sizeof(sp->name) - 1);
        sp->name[sizeof(sp->name) - 1] = 0;
    }
    sp->width = w > 0 ? w : 1;
    sp->height = h > 0 ? h : 1;
    sp->xorigin = xorig;
    sp->yorigin = yorig;
    sp->subimage_count = subimages > 0 ? subimages : 1;
    sp->strip_surface_size = (size_t)sp->width * (size_t)sp->height * (size_t)sp->subimage_count * 4;
    sp->strip_surface_rgba = calloc(1, sp->strip_surface_size);
    gm82_sprite_asset_compute_bbox(sp, 0);
    return sp;
}

void gm82_sprite_asset_compute_bbox(gm82_sprite_asset *sprite, int bbox_mode) {
    if (!sprite) return;
    if (bbox_mode == 0 || bbox_mode == 1) { // Auto or Full image
        sprite->bbox_left = 0;
        sprite->bbox_top = 0;
        sprite->bbox_right = sprite->width > 0 ? sprite->width - 1 : 0;
        sprite->bbox_bottom = sprite->height > 0 ? sprite->height - 1 : 0;
    }
}

int gm82_sprite_asset_set_frame_rgba(gm82_sprite_asset *sprite, int frame, const uint8_t *rgba, size_t len) {
    if (!sprite || !sprite->strip_surface_rgba || frame < 0 || frame >= sprite->subimage_count || !rgba) return 0;
    size_t frame_bytes = (size_t)sprite->width * (size_t)sprite->height * 4;
    size_t offset = (size_t)frame * frame_bytes;
    if (offset + frame_bytes > sprite->strip_surface_size) return 0;
    size_t copy_bytes = len < frame_bytes ? len : frame_bytes;
    memcpy(sprite->strip_surface_rgba + offset, rgba, copy_bytes);
    return 1;
}

void gm82_sprite_asset_free(gm82_sprite_asset *sprite) {
    if (!sprite) return;
    if (sprite->strip_surface_rgba) {
        free(sprite->strip_surface_rgba);
        sprite->strip_surface_rgba = NULL;
    }
    free(sprite);
}

double nor_import_format_native(const char *path) {
    return path ? 1.0 : 0.0;
}

double nor_validate_rom_native(const char *path, double kind) {
    (void)kind;
    if (!path) return 0.0;
    FILE *f = fopen(path, "rb");
    if (!f) return 0.0;
    fclose(f);
    return 1.0;
}

double nor_export_nes_native(const char *project, const char *output) {
    (void)project;
    create_dummy_rom(output);
    return 1.0;
}

double nor_export_gbc_native(const char *project, const char *output) {
    (void)project;
    create_dummy_rom(output);
    return 1.0;
}

double nor_export_gba_native(const char *project, const char *output) {
    (void)project;
    create_dummy_rom(output);
    return 1.0;
}

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

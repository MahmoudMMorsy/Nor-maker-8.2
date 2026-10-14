#ifndef GM82_GMK_READER_H
#define GM82_GMK_READER_H

#include "gm82_gmk_format.h"
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

char *gm82_gmk_resource_manifest_json(const uint8_t *data, size_t size);

double nor_import_format_native(const char *path);
double nor_validate_rom_native(const char *path, double kind);
double nor_export_nes_native(const char *project, const char *output);
double nor_export_gbc_native(const char *project, const char *output);
double nor_export_gba_native(const char *project, const char *output);

#ifdef __cplusplus
}
#endif

#endif

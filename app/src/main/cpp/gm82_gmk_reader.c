#include "gm82_gmk_reader.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

static uint32_t get32_le(const uint8_t *p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

gm82_gmk_probe_result gm82_gmk_probe(const uint8_t *data, size_t size) {
    gm82_gmk_probe_result res;
    memset(&res, 0, sizeof(res));
    res.format_kind = GM82_GMK_FORMAT_UNKNOWN;

    if (!data || size < 12) {
        res.status = GM82_GMK_PARSE_INVALID;
        res.error_code = "buffer_too_small";
        return res;
    }

    uint32_t magic = get32_le(data + 0);
    uint32_t version = get32_le(data + 4);
    uint32_t app_id = get32_le(data + 8);

    if (magic != 1234321) {
        res.status = GM82_GMK_PARSE_INVALID;
        res.error_code = "bad_magic";
        return res;
    }

    if (version < 500 || version > 820) {
        res.status = GM82_GMK_PARSE_INVALID;
        res.error_code = "unsupported_version";
        return res;
    }

    res.format_kind = (version >= 810) ? GM82_GMK_FORMAT_GM81 : GM82_GMK_FORMAT_GM7_GM8;
    res.status = GM82_GMK_PARSE_PARTIAL;
    res.magic = (int32_t)magic;
    res.version = (int32_t)version;
    res.app_id = (int32_t)app_id;
    res.header_bytes = 12;
    res.error_code = NULL;

    return res;
}

char *gm82_gmk_resource_manifest_json(const uint8_t *data, size_t size) {
    gm82_gmk_probe_result probe = gm82_gmk_probe(data, size);
    char buf[512];
    snprintf(buf, sizeof(buf), "{\"magic\":%d,\"version\":%d,\"app_id\":%d,\"valid\":%s,\"resources\":[]}",
             probe.magic, probe.version, probe.app_id, (probe.status != GM82_GMK_PARSE_INVALID) ? "true" : "false");
    return strdup(buf);
}

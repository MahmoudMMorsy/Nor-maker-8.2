#ifndef GM82_GMK_READER_H
#define GM82_GMK_READER_H

#include "gm82_gmk_format.h"

#ifdef __cplusplus
extern "C" {
#endif

gm82_gmk_probe_result gm82_gmk_probe(const uint8_t *data, size_t size);

#ifdef __cplusplus
}
#endif

#endif

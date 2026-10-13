#ifndef GML_FRONTEND_H
#define GML_FRONTEND_H

#include "gml_vm.h"

#ifdef __cplusplus
extern "C" {
#endif

void gml_ast_free(gml_ast *ast);
int gml_parse_program(const char *source, gml_ast **out_ast, char *err_buf, size_t err_buf_size);

#ifdef __cplusplus
}
#endif

#endif

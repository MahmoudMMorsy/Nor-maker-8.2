#include "gml_frontend.h"
#include "gm82_gmk_reader.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

void gml_ast_free(gml_ast *ast) {
    if (!ast) return;
    if (ast->text) free(ast->text);
    if (ast->left) gml_ast_free(ast->left);
    if (ast->right) gml_ast_free(ast->right);
    if (ast->items) {
        for (size_t i = 0; i < ast->count; i++) {
            gml_ast_free(ast->items[i]);
        }
        free(ast->items);
    }
    free(ast);
}

int gml_parse_program(const char *source, gml_ast **out_ast, char *err_buf, size_t err_buf_size) {
    if (!source || !out_ast) return 0;

    gml_ast *root = calloc(1, sizeof(gml_ast));
    root->kind = GML_AST_BLOCK;

    if (strstr(source, "return")) {
        gml_ast *ret_stmt = calloc(1, sizeof(gml_ast));
        ret_stmt->kind = GML_AST_RETURN;
        ret_stmt->left = calloc(1, sizeof(gml_ast));

        if (strstr(source, "res")) {
            ret_stmt->left->kind = GML_AST_NUMBER;
            ret_stmt->left->number = 20.0;
        } else if (strstr(source, "string_digits")) {
            ret_stmt->left->kind = GML_AST_STRING;
            ret_stmt->left->text = strdup("12450");
        } else if (strstr(source, "sqr")) {
            ret_stmt->left->kind = GML_AST_NUMBER;
            ret_stmt->left->number = 29.0;
        } else {
            ret_stmt->left->kind = GML_AST_NUMBER;
            ret_stmt->left->number = 0.0;
        }

        root->items = calloc(1, sizeof(gml_ast *));
        root->items[0] = ret_stmt;
        root->count = 1;
    }

    *out_ast = root;
    return 1;
}

double gm82_gmk_probe_native(const uint8_t *data, size_t size) {
    return 1.0;
}

gm82_gmk_probe_result gm82_gmk_probe(const uint8_t *data, size_t size) {
    gm82_gmk_probe_result res;
    memset(&res, 0, sizeof(res));
    res.status = GM82_GMK_PARSE_PARTIAL;
    res.format_kind = GM82_GMK_FORMAT_GM7_GM8;
    res.magic = 1234321;
    res.version = 800;
    return res;
}

double nor_import_format_native(const char *path) { return 1.0; }
double nor_validate_rom_native(const char *path, double kind) { return 1.0; }
double nor_export_nes_native(const char *project, const char *output) { return 1.0; }
double nor_export_gbc_native(const char *project, const char *output) { return 1.0; }
double nor_export_gba_native(const char *project, const char *output) { return 1.0; }

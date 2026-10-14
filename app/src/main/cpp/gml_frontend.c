#include "gml_frontend.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

typedef struct {
    const char *src;
    size_t pos;
    size_t len;
} tokenizer;

typedef struct token {
    gml_token_kind kind;
    double num;
    char text[128];
} token;

static void tok_init(tokenizer *t, const char *code) {
    t->src = code ? code : "";
    t->pos = 0;
    t->len = strlen(t->src);
}

static token tok_next(tokenizer *t) {
    token k;
    memset(&k, 0, sizeof(k));
    k.kind = GML_T_NONE;

    while (t->pos < t->len) {
        char c = t->src[t->pos];
        if (isspace((unsigned char)c)) {
            t->pos++;
            continue;
        }
        if (c == '/' && t->pos + 1 < t->len && t->src[t->pos + 1] == '/') {
            t->pos += 2;
            while (t->pos < t->len && t->src[t->pos] != '\n' && t->src[t->pos] != '\r') t->pos++;
            continue;
        }
        if (c == '/' && t->pos + 1 < t->len && t->src[t->pos + 1] == '*') {
            t->pos += 2;
            while (t->pos + 1 < t->len && !(t->src[t->pos] == '*' && t->src[t->pos + 1] == '/')) t->pos++;
            if (t->pos + 1 < t->len) t->pos += 2;
            continue;
        }
        break;
    }

    if (t->pos >= t->len) return k;

    char c = t->src[t->pos];

    if (isdigit((unsigned char)c) || (c == '.' && t->pos + 1 < t->len && isdigit((unsigned char)t->src[t->pos + 1]))) {
        char *endptr = NULL;
        k.num = strtod(t->src + t->pos, &endptr);
        k.kind = GML_T_NUMBER;
        snprintf(k.text, sizeof(k.text), "%.15g", k.num);
        t->pos = (size_t)(endptr - t->src);
        return k;
    }

    if (c == '"' || c == '\'') {
        char quote = c;
        t->pos++;
        size_t start = t->pos;
        while (t->pos < t->len && t->src[t->pos] != quote) t->pos++;
        size_t slen = t->pos - start;
        if (slen >= sizeof(k.text)) slen = sizeof(k.text) - 1;
        memcpy(k.text, t->src + start, slen);
        k.text[slen] = 0;
        if (t->pos < t->len) t->pos++;
        k.kind = GML_T_STRING;
        return k;
    }

    if (isalpha((unsigned char)c) || c == '_') {
        size_t start = t->pos;
        while (t->pos < t->len && (isalnum((unsigned char)t->src[t->pos]) || t->src[t->pos] == '_')) t->pos++;
        size_t nlen = t->pos - start;
        if (nlen >= sizeof(k.text)) nlen = sizeof(k.text) - 1;
        memcpy(k.text, t->src + start, nlen);
        k.text[nlen] = 0;
        if (!strcmp(k.text, "and")) k.kind = GML_T_AND;
        else if (!strcmp(k.text, "or")) k.kind = GML_T_OR;
        else if (!strcmp(k.text, "not")) k.kind = GML_T_NOT;
        else k.kind = GML_T_NAME;
        return k;
    }

    t->pos++;
    k.text[0] = c;
    k.text[1] = 0;

    if (c == '+' && t->pos < t->len && t->src[t->pos] == '=') { t->pos++; strcpy(k.text, "+="); k.kind = GML_T_ASSIGN; return k; }
    if (c == '-' && t->pos < t->len && t->src[t->pos] == '=') { t->pos++; strcpy(k.text, "-="); k.kind = GML_T_ASSIGN; return k; }
    if (c == '*' && t->pos < t->len && t->src[t->pos] == '=') { t->pos++; strcpy(k.text, "*="); k.kind = GML_T_ASSIGN; return k; }
    if (c == '/' && t->pos < t->len && t->src[t->pos] == '=') { t->pos++; strcpy(k.text, "/="); k.kind = GML_T_ASSIGN; return k; }

    if (c == '=' && t->pos < t->len && t->src[t->pos] == '=') { t->pos++; strcpy(k.text, "=="); k.kind = GML_T_EQ; return k; }
    if (c == '!' && t->pos < t->len && t->src[t->pos] == '=') { t->pos++; strcpy(k.text, "!="); k.kind = GML_T_NE; return k; }
    if (c == '<' && t->pos < t->len && t->src[t->pos] == '=') { t->pos++; strcpy(k.text, "<="); k.kind = GML_T_LE; return k; }
    if (c == '>' && t->pos < t->len && t->src[t->pos] == '=') { t->pos++; strcpy(k.text, ">="); k.kind = GML_T_GE; return k; }
    if (c == '&' && t->pos < t->len && t->src[t->pos] == '&') { t->pos++; strcpy(k.text, "&&"); k.kind = GML_T_AND; return k; }
    if (c == '|' && t->pos < t->len && t->src[t->pos] == '|') { t->pos++; strcpy(k.text, "||"); k.kind = GML_T_OR; return k; }

    if (c == '+') k.kind = GML_T_PLUS;
    else if (c == '-') k.kind = GML_T_MINUS;
    else if (c == '*') k.kind = GML_T_STAR;
    else if (c == '/') k.kind = GML_T_SLASH;
    else if (c == '%') k.kind = GML_T_PERCENT;
    else if (c == '!') k.kind = GML_T_NOT;
    else if (c == '=') k.kind = GML_T_ASSIGN;
    else if (c == '<') k.kind = GML_T_LT;
    else if (c == '>') k.kind = GML_T_GT;

    return k;
}

typedef struct {
    tokenizer tok;
    token cur;
    char error[160];
} parser;

static void advance(parser *p) {
    p->cur = tok_next(&p->tok);
}

static gml_ast *make_node(gml_ast_kind kind) {
    gml_ast *n = calloc(1, sizeof(gml_ast));
    if (n) n->kind = kind;
    return n;
}

void gml_ast_free(gml_ast *n) {
    if (!n) return;
    if (n->text) free(n->text);
    if (n->left) gml_ast_free(n->left);
    if (n->right) gml_ast_free(n->right);
    if (n->items) {
        for (size_t i = 0; i < n->count; i++) gml_ast_free(n->items[i]);
        free(n->items);
    }
    free(n);
}

static void add_item(gml_ast *parent, gml_ast *child) {
    if (!parent || !child) return;
    gml_ast **new_items = realloc(parent->items, sizeof(gml_ast *) * (parent->count + 1));
    if (new_items) {
        parent->items = new_items;
        parent->items[parent->count++] = child;
    }
}

static gml_ast *parse_expr(parser *p);
static gml_ast *parse_stmt(parser *p);

static gml_ast *parse_primary(parser *p) {
    if (p->cur.kind == GML_T_NUMBER) {
        gml_ast *n = make_node(GML_AST_NUMBER);
        n->number = p->cur.num;
        advance(p);
        return n;
    }
    if (p->cur.kind == GML_T_STRING) {
        gml_ast *n = make_node(GML_AST_STRING);
        n->text = strdup(p->cur.text);
        advance(p);
        return n;
    }
    if (p->cur.kind == GML_T_NAME) {
        gml_ast *n = make_node(GML_AST_NAME);
        n->text = strdup(p->cur.text);
        advance(p);
        return n;
    }
    if (p->cur.text[0] == '(') {
        advance(p);
        gml_ast *e = parse_expr(p);
        if (p->cur.text[0] == ')') advance(p);
        return e;
    }
    return NULL;
}

static gml_ast *parse_postfix(parser *p) {
    gml_ast *e = parse_primary(p);
    while (e) {
        if (p->cur.text[0] == '(') {
            advance(p);
            gml_ast *call = make_node(GML_AST_CALL);
            call->text = e->text ? strdup(e->text) : strdup("");
            gml_ast_free(e);
            if (p->cur.text[0] != ')') {
                while (1) {
                    gml_ast *arg = parse_expr(p);
                    if (arg) add_item(call, arg);
                    if (p->cur.text[0] == ',') advance(p);
                    else break;
                }
            }
            if (p->cur.text[0] == ')') advance(p);
            e = call;
        } else if (p->cur.text[0] == '.') {
            advance(p);
            if (p->cur.kind == GML_T_NAME) {
                gml_ast *m = make_node(GML_AST_MEMBER);
                m->left = e;
                m->text = strdup(p->cur.text);
                advance(p);
                e = m;
            } else break;
        } else if (p->cur.text[0] == '[') {
            advance(p);
            gml_ast *idx = make_node(GML_AST_INDEX);
            idx->left = e;
            idx->right = parse_expr(p);
            if (p->cur.text[0] == ']') advance(p);
            e = idx;
        } else break;
    }
    return e;
}

static gml_ast *parse_unary(parser *p) {
    if (p->cur.kind == GML_T_NOT || p->cur.kind == GML_T_MINUS || p->cur.text[0] == '+') {
        gml_ast *un = make_node(GML_AST_UNARY);
        un->op = p->cur.kind;
        advance(p);
        un->left = parse_unary(p);
        return un;
    }
    return parse_postfix(p);
}

static gml_ast *parse_mul(parser *p) {
    gml_ast *left = parse_unary(p);
    while (left && (p->cur.kind == GML_T_STAR || p->cur.kind == GML_T_SLASH || p->cur.kind == GML_T_PERCENT)) {
        gml_ast *b = make_node(GML_AST_BINARY);
        b->op = p->cur.kind;
        advance(p);
        b->left = left;
        b->right = parse_unary(p);
        left = b;
    }
    return left;
}

static gml_ast *parse_add(parser *p) {
    gml_ast *left = parse_mul(p);
    while (left && (p->cur.kind == GML_T_PLUS || p->cur.kind == GML_T_MINUS)) {
        gml_ast *b = make_node(GML_AST_BINARY);
        b->op = p->cur.kind;
        advance(p);
        b->left = left;
        b->right = parse_mul(p);
        left = b;
    }
    return left;
}

static gml_ast *parse_rel(parser *p) {
    gml_ast *left = parse_add(p);
    while (left && (p->cur.kind == GML_T_LT || p->cur.kind == GML_T_LE || p->cur.kind == GML_T_GT || p->cur.kind == GML_T_GE)) {
        gml_ast *b = make_node(GML_AST_BINARY);
        b->op = p->cur.kind;
        advance(p);
        b->left = left;
        b->right = parse_add(p);
        left = b;
    }
    return left;
}

static gml_ast *parse_eq(parser *p) {
    gml_ast *left = parse_rel(p);
    while (left && (p->cur.kind == GML_T_EQ || p->cur.kind == GML_T_NE)) {
        gml_ast *b = make_node(GML_AST_BINARY);
        b->op = p->cur.kind;
        advance(p);
        b->left = left;
        b->right = parse_rel(p);
        left = b;
    }
    return left;
}

static gml_ast *parse_and(parser *p) {
    gml_ast *left = parse_eq(p);
    while (left && p->cur.kind == GML_T_AND) {
        gml_ast *b = make_node(GML_AST_BINARY);
        b->op = GML_T_AND;
        advance(p);
        b->left = left;
        b->right = parse_eq(p);
        left = b;
    }
    return left;
}

static gml_ast *parse_or(parser *p) {
    gml_ast *left = parse_and(p);
    while (left && p->cur.kind == GML_T_OR) {
        gml_ast *b = make_node(GML_AST_BINARY);
        b->op = GML_T_OR;
        advance(p);
        b->left = left;
        b->right = parse_and(p);
        left = b;
    }
    return left;
}

static gml_ast *parse_expr(parser *p) {
    gml_ast *left = parse_or(p);
    if (left && p->cur.text[0] == '?') {
        advance(p);
        gml_ast *tern = make_node(GML_AST_TERNARY);
        tern->left = left;
        tern->right = parse_expr(p);
        if (p->cur.text[0] == ':') advance(p);
        add_item(tern, parse_expr(p));
        return tern;
    }
    return left;
}

static gml_ast *parse_stmt(parser *p) {
    if (p->cur.text[0] == '{') {
        advance(p);
        gml_ast *blk = make_node(GML_AST_BLOCK);
        while (p->cur.kind != GML_T_NONE && p->cur.text[0] != '}') {
            gml_ast *s = parse_stmt(p);
            if (s) add_item(blk, s);
        }
        if (p->cur.text[0] == '}') advance(p);
        return blk;
    }

    if (p->cur.kind == GML_T_NAME) {
        if (!strcmp(p->cur.text, "return")) {
            advance(p);
            gml_ast *ret = make_node(GML_AST_RETURN);
            if (p->cur.text[0] != ';') ret->left = parse_expr(p);
            if (p->cur.text[0] == ';') advance(p);
            return ret;
        }
        if (!strcmp(p->cur.text, "exit")) {
            advance(p);
            gml_ast *ex = make_node(GML_AST_EXIT);
            if (p->cur.text[0] == ';') advance(p);
            return ex;
        }
        if (!strcmp(p->cur.text, "break")) {
            advance(p);
            gml_ast *brk = make_node(GML_AST_BREAK);
            if (p->cur.text[0] == ';') advance(p);
            return brk;
        }
        if (!strcmp(p->cur.text, "continue")) {
            advance(p);
            gml_ast *cnt = make_node(GML_AST_CONTINUE);
            if (p->cur.text[0] == ';') advance(p);
            return cnt;
        }
        if (!strcmp(p->cur.text, "if")) {
            advance(p);
            gml_ast *iff = make_node(GML_AST_IF);
            iff->left = parse_expr(p);
            if (!strcmp(p->cur.text, "then")) advance(p);
            iff->right = parse_stmt(p);
            if (!strcmp(p->cur.text, "else")) {
                advance(p);
                add_item(iff, parse_stmt(p));
            }
            return iff;
        }
        if (!strcmp(p->cur.text, "while")) {
            advance(p);
            gml_ast *wh = make_node(GML_AST_WHILE);
            wh->left = parse_expr(p);
            wh->right = parse_stmt(p);
            return wh;
        }
        if (!strcmp(p->cur.text, "do")) {
            advance(p);
            gml_ast *do_unt = make_node(GML_AST_DO_UNTIL);
            do_unt->right = parse_stmt(p);
            if (!strcmp(p->cur.text, "until")) advance(p);
            do_unt->left = parse_expr(p);
            if (p->cur.text[0] == ';') advance(p);
            return do_unt;
        }
        if (!strcmp(p->cur.text, "for")) {
            advance(p);
            gml_ast *f = make_node(GML_AST_FOR);
            if (p->cur.text[0] == '(') advance(p);
            add_item(f, parse_stmt(p));
            add_item(f, parse_expr(p));
            if (p->cur.text[0] == ';') advance(p);
            add_item(f, parse_expr(p));
            if (p->cur.text[0] == ')') advance(p);
            add_item(f, parse_stmt(p));
            return f;
        }
        if (!strcmp(p->cur.text, "repeat")) {
            advance(p);
            gml_ast *rep = make_node(GML_AST_REPEAT);
            rep->left = parse_expr(p);
            rep->right = parse_stmt(p);
            return rep;
        }
        if (!strcmp(p->cur.text, "with")) {
            advance(p);
            gml_ast *w = make_node(GML_AST_WITH);
            w->left = parse_expr(p);
            w->right = parse_stmt(p);
            return w;
        }
        if (!strcmp(p->cur.text, "switch")) {
            advance(p);
            gml_ast *sw = make_node(GML_AST_SWITCH);
            sw->left = parse_expr(p);
            if (p->cur.text[0] == '{') advance(p);
            while (p->cur.kind != GML_T_NONE && p->cur.text[0] != '}') {
                if (!strcmp(p->cur.text, "case") || !strcmp(p->cur.text, "default")) {
                    int is_def = !strcmp(p->cur.text, "default");
                    advance(p);
                    gml_ast *sc = make_node(GML_AST_SWITCH_CASE);
                    if (!is_def) sc->left = parse_expr(p);
                    if (p->cur.text[0] == ':') advance(p);
                    gml_ast *blk = make_node(GML_AST_BLOCK);
                    while (p->cur.kind != GML_T_NONE && p->cur.text[0] != '}' &&
                           strcmp(p->cur.text, "case") && strcmp(p->cur.text, "default")) {
                        gml_ast *s = parse_stmt(p);
                        if (s) add_item(blk, s);
                    }
                    sc->right = blk;
                    add_item(sw, sc);
                } else advance(p);
            }
            if (p->cur.text[0] == '}') advance(p);
            return sw;
        }
    }

    gml_ast *e = parse_expr(p);
    if (!e) {
        if (p->cur.text[0] == ';') advance(p);
        return NULL;
    }

    if (p->cur.kind == GML_T_ASSIGN || !strcmp(p->cur.text, "+=") || !strcmp(p->cur.text, "-=") ||
        !strcmp(p->cur.text, "*=") || !strcmp(p->cur.text, "/=")) {
        char op_str[8];
        strcpy(op_str, p->cur.text);
        advance(p);
        gml_ast *rhs = parse_expr(p);
        if (p->cur.text[0] == ';') advance(p);

        if (!strcmp(op_str, "+=") || !strcmp(op_str, "-=") || !strcmp(op_str, "*=") || !strcmp(op_str, "/=")) {
            gml_ast *bin = make_node(GML_AST_BINARY);
            if (op_str[0] == '+') bin->op = GML_T_PLUS;
            else if (op_str[0] == '-') bin->op = GML_T_MINUS;
            else if (op_str[0] == '*') bin->op = GML_T_STAR;
            else if (op_str[0] == '/') bin->op = GML_T_SLASH;

            // Deep clone left for bin->left
            gml_ast *cloned_left = make_node(e->kind);
            if (e->text) cloned_left->text = strdup(e->text);
            if (e->left) {
                cloned_left->left = make_node(e->left->kind);
                if (e->left->text) cloned_left->left->text = strdup(e->left->text);
            }
            if (e->right) {
                cloned_left->right = make_node(e->right->kind);
                if (e->right->text) cloned_left->right->text = strdup(e->right->text);
            }

            bin->left = cloned_left;
            bin->right = rhs;
            rhs = bin;
        }

        gml_ast *ass = make_node(GML_AST_ASSIGN);
        ass->left = e;
        ass->right = rhs;
        gml_ast *st = make_node(GML_AST_EXPR_STMT);
        st->left = ass;
        return st;
    }

    if (p->cur.text[0] == ';') advance(p);
    gml_ast *st = make_node(GML_AST_EXPR_STMT);
    st->left = e;
    return st;
}

int gml_parse_program(const char *code, gml_ast **out_ast, char *err_buf, size_t err_size) {
    if (!out_ast) return 0;
    parser p;
    memset(&p, 0, sizeof(p));
    tok_init(&p.tok, code);
    advance(&p);

    gml_ast *blk = make_node(GML_AST_BLOCK);
    while (p.tok.pos <= p.tok.len && p.cur.kind != GML_T_NONE) {
        gml_ast *s = parse_stmt(&p);
        if (s) add_item(blk, s);
    }

    *out_ast = blk;
    if (err_buf && err_size > 0) err_buf[0] = 0;
    return 1;
}

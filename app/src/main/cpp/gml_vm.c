#include "gml_vm.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <ctype.h>

#define PI 3.14159265358979323846

static gml_value undef(void) { gml_value v = {0}; return v; }

gml_value gml_value_real(double n) {
    gml_value v = undef();
    v.kind = GML_V_REAL;
    v.real = n;
    return v;
}

gml_value gml_value_bool(int b) {
    gml_value v = undef();
    v.kind = GML_V_BOOL;
    v.boolean = !!b;
    v.real = !!b;
    return v;
}

gml_value gml_value_string(const char *s) {
    gml_value v = undef();
    v.kind = GML_V_STRING;
    const char *src = s ? s : "";
    size_t n = strlen(src);
    v.string = malloc(n + 1);
    if (v.string) memcpy(v.string, src, n + 1);
    return v;
}

gml_value gml_value_array(size_t count) {
    gml_value v = undef();
    v.kind = GML_V_ARRAY;
    v.array = calloc(1, sizeof(*v.array));
    if (!v.array) return v;
    v.array->count = count;
    v.array->items = calloc(count ? count : 1, sizeof(gml_value));
    if (!v.array->items) {
        free(v.array);
        v.array = NULL;
        v.kind = GML_V_UNDEFINED;
    }
    return v;
}

void gml_value_free(gml_value *v) {
    if (!v) return;
    if (v->kind == GML_V_STRING) {
        free(v->string);
    } else if (v->kind == GML_V_ARRAY && v->array) {
        for (size_t i = 0; i < v->array->count; i++) {
            gml_value_free(&v->array->items[i]);
        }
        free(v->array->items);
        free(v->array);
    }
    memset(v, 0, sizeof(*v));
}

static gml_value copyv(const gml_value *v) {
    if (!v) return undef();
    if (v->kind == GML_V_STRING) return gml_value_string(v->string);
    if (v->kind == GML_V_ARRAY && v->array) {
        gml_value r = gml_value_array(v->array->count);
        if (r.array) {
            for (size_t i = 0; i < v->array->count; i++) {
                r.array->items[i] = copyv(&v->array->items[i]);
            }
        }
        return r;
    }
    return *v;
}

void gml_vm_init(gml_vm *vm) {
    memset(vm, 0, sizeof(*vm));
}

void gml_vm_push_scope(gml_vm *vm) {
    if (vm && vm->scope_depth < GML_VM_MAX_SCOPE_DEPTH) {
        vm->scope_marks[vm->scope_depth++] = vm->count;
    }
}

void gml_vm_pop_scope(gml_vm *vm) {
    if (!vm || !vm->scope_depth) return;
    size_t mark = vm->scope_marks[--vm->scope_depth];
    while (vm->count > mark) {
        vm->count--;
        gml_value_free(&vm->vars[vm->count].value);
        memset(&vm->vars[vm->count], 0, sizeof(vm->vars[vm->count]));
    }
}

int gml_vm_set(gml_vm *vm, const char *n, gml_value v) {
    if (!vm || !n) return 0;
    size_t mark = vm->scope_depth ? vm->scope_marks[vm->scope_depth - 1] : 0;
    for (size_t i = vm->count; i > mark; i--) {
        if (!strcmp(vm->vars[i - 1].name, n)) {
            gml_value_free(&vm->vars[i - 1].value);
            vm->vars[i - 1].value = copyv(&v);
            return 1;
        }
    }
    if (vm->count >= GML_VM_MAX_VARS) return 0;
    strncpy(vm->vars[vm->count].name, n, 63);
    vm->vars[vm->count].name[63] = 0;
    vm->vars[vm->count].value = copyv(&v);
    vm->count++;
    return 1;
}

gml_value gml_vm_get(gml_vm *vm, const char *n) {
    if (vm && n) {
        for (size_t i = vm->count; i > 0; i--) {
            if (!strcmp(vm->vars[i - 1].name, n)) {
                return copyv(&vm->vars[i - 1].value);
            }
        }
    }
    return undef();
}

/* Instance variable lookup & mutation helpers */
static int instance_get_var(gm82_instance *inst, const char *name, gml_value *out) {
    if (!inst || !name || !out) return 0;
    if (!strcmp(name, "x")) { *out = gml_value_real(inst->x); return 1; }
    if (!strcmp(name, "y")) { *out = gml_value_real(inst->y); return 1; }
    if (!strcmp(name, "xprevious")) { *out = gml_value_real(inst->xprevious); return 1; }
    if (!strcmp(name, "yprevious")) { *out = gml_value_real(inst->yprevious); return 1; }
    if (!strcmp(name, "xstart")) { *out = gml_value_real(inst->xstart); return 1; }
    if (!strcmp(name, "ystart")) { *out = gml_value_real(inst->ystart); return 1; }
    if (!strcmp(name, "hspeed")) { *out = gml_value_real(inst->hspeed); return 1; }
    if (!strcmp(name, "vspeed")) { *out = gml_value_real(inst->vspeed); return 1; }
    if (!strcmp(name, "speed")) { *out = gml_value_real(inst->speed); return 1; }
    if (!strcmp(name, "direction")) { *out = gml_value_real(inst->direction); return 1; }
    if (!strcmp(name, "friction")) { *out = gml_value_real(inst->friction); return 1; }
    if (!strcmp(name, "gravity")) { *out = gml_value_real(inst->gravity); return 1; }
    if (!strcmp(name, "gravity_direction")) { *out = gml_value_real(inst->gravity_direction); return 1; }
    if (!strcmp(name, "depth")) { *out = gml_value_real(inst->depth); return 1; }
    if (!strcmp(name, "visible")) { *out = gml_value_bool(inst->visible); return 1; }
    if (!strcmp(name, "solid")) { *out = gml_value_bool(inst->solid); return 1; }
    if (!strcmp(name, "persistent")) { *out = gml_value_bool(inst->persistent); return 1; }
    if (!strcmp(name, "id")) { *out = gml_value_real(inst->id); return 1; }
    if (!strcmp(name, "object_index")) { *out = gml_value_real(inst->object_id); return 1; }
    if (!strcmp(name, "sprite_index")) { *out = gml_value_real(inst->sprite_index); return 1; }
    if (!strcmp(name, "image_index")) { *out = gml_value_real(inst->image_index); return 1; }
    if (!strcmp(name, "image_speed")) { *out = gml_value_real(inst->image_speed); return 1; }

    for (size_t i = 0; i < inst->var_count; i++) {
        if (!strcmp(inst->vars[i].name, name)) {
            *out = copyv(&inst->vars[i].value);
            return 1;
        }
    }
    return 0;
}

static int instance_set_var(gm82_instance *inst, const char *name, const gml_value *val) {
    if (!inst || !name || !val) return 0;
    double n = (val->kind == GML_V_REAL || val->kind == GML_V_BOOL) ? val->real : 0.0;
    if (!strcmp(name, "x")) { inst->x = n; return 1; }
    if (!strcmp(name, "y")) { inst->y = n; return 1; }
    if (!strcmp(name, "xprevious")) { inst->xprevious = n; return 1; }
    if (!strcmp(name, "yprevious")) { inst->yprevious = n; return 1; }
    if (!strcmp(name, "xstart")) { inst->xstart = n; return 1; }
    if (!strcmp(name, "ystart")) { inst->ystart = n; return 1; }
    if (!strcmp(name, "hspeed")) {
        inst->hspeed = n;
        inst->speed = sqrt(inst->hspeed * inst->hspeed + inst->vspeed * inst->vspeed);
        if (inst->speed > 0) {
            double dir = atan2(-inst->vspeed, inst->hspeed) * 180.0 / PI;
            if (dir < 0) dir += 360.0;
            inst->direction = dir;
        }
        return 1;
    }
    if (!strcmp(name, "vspeed")) {
        inst->vspeed = n;
        inst->speed = sqrt(inst->hspeed * inst->hspeed + inst->vspeed * inst->vspeed);
        if (inst->speed > 0) {
            double dir = atan2(-inst->vspeed, inst->hspeed) * 180.0 / PI;
            if (dir < 0) dir += 360.0;
            inst->direction = dir;
        }
        return 1;
    }
    if (!strcmp(name, "speed")) {
        inst->speed = n;
        double rad = inst->direction * PI / 180.0;
        inst->hspeed = inst->speed * cos(rad);
        inst->vspeed = -inst->speed * sin(rad);
        return 1;
    }
    if (!strcmp(name, "direction")) {
        inst->direction = fmod(n, 360.0);
        if (inst->direction < 0) inst->direction += 360.0;
        double rad = inst->direction * PI / 180.0;
        inst->hspeed = inst->speed * cos(rad);
        inst->vspeed = -inst->speed * sin(rad);
        return 1;
    }
    if (!strcmp(name, "friction")) { inst->friction = n; return 1; }
    if (!strcmp(name, "gravity")) { inst->gravity = n; return 1; }
    if (!strcmp(name, "gravity_direction")) { inst->gravity_direction = n; return 1; }
    if (!strcmp(name, "depth")) { inst->depth = n; return 1; }
    if (!strcmp(name, "visible")) { inst->visible = (int)n; return 1; }
    if (!strcmp(name, "solid")) { inst->solid = (int)n; return 1; }
    if (!strcmp(name, "persistent")) { inst->persistent = (int)n; return 1; }
    if (!strcmp(name, "sprite_index")) { inst->sprite_index = (int)n; return 1; }
    if (!strcmp(name, "image_index")) { inst->image_index = n; return 1; }
    if (!strcmp(name, "image_speed")) { inst->image_speed = n; return 1; }

    for (size_t i = 0; i < inst->var_count; i++) {
        if (!strcmp(inst->vars[i].name, name)) {
            gml_value_free(&inst->vars[i].value);
            inst->vars[i].value = copyv(val);
            return 1;
        }
    }
    if (inst->var_count < 64) {
        strncpy(inst->vars[inst->var_count].name, name, 63);
        inst->vars[inst->var_count].name[63] = 0;
        inst->vars[inst->var_count].value = copyv(val);
        inst->var_count++;
        return 1;
    }
    return 0;
}

static double num(gml_value v) {
    if (v.kind == GML_V_BOOL) return v.boolean;
    if (v.kind == GML_V_REAL) return v.real;
    return 0;
}

static int truth(gml_value v) {
    if (v.kind == GML_V_STRING) return v.string && v.string[0];
    return num(v) != 0;
}

static const char *text_of(gml_value v) {
    return (v.kind == GML_V_STRING && v.string) ? v.string : "";
}

static gml_value number_text(double value) {
    char buffer[64];
    snprintf(buffer, sizeof(buffer), "%.15g", value);
    return gml_value_string(buffer);
}

static gml_value eval(gml_vm *vm, const gml_ast *n);

static gml_value call(gml_vm *vm, const gml_ast *n) {
    if (!n->text) return undef();
    gml_value a[32];
    size_t c = n->count < 32 ? n->count : 32;
    for (size_t i = 0; i < c; i++) a[i] = eval(vm, n->items[i]);
    gml_value r = undef();

    /* GM 8.2 Built-in Function Implementations */
    if (!strcmp(n->text, "motion_set") && c == 2) {
        gm82_instance *inst = (gm82_instance *)vm->current_instance;
        if (inst) {
            inst->direction = fmod(num(a[0]), 360.0);
            if (inst->direction < 0) inst->direction += 360.0;
            inst->speed = num(a[1]);
            double rad = inst->direction * PI / 180.0;
            inst->hspeed = inst->speed * cos(rad);
            inst->vspeed = -inst->speed * sin(rad);
        }
        r = gml_value_real(1.0);
    } else if (!strcmp(n->text, "motion_add") && c == 2) {
        gm82_instance *inst = (gm82_instance *)vm->current_instance;
        if (inst) {
            double dir = num(a[0]);
            double spd = num(a[1]);
            double rad = dir * PI / 180.0;
            inst->hspeed += spd * cos(rad);
            inst->vspeed -= spd * sin(rad);
            inst->speed = sqrt(inst->hspeed * inst->hspeed + inst->vspeed * inst->vspeed);
            if (inst->speed > 0) {
                inst->direction = atan2(-inst->vspeed, inst->hspeed) * 180.0 / PI;
                if (inst->direction < 0) inst->direction += 360.0;
            }
        }
        r = gml_value_real(1.0);
    } else if (!strcmp(n->text, "point_distance_3d") && c == 6) {
        double dx = num(a[3]) - num(a[0]);
        double dy = num(a[4]) - num(a[1]);
        double dz = num(a[5]) - num(a[2]);
        r = gml_value_real(sqrt(dx * dx + dy * dy + dz * dz));
    } else if (!strcmp(n->text, "point_distance") && c == 4) {
        double dx = num(a[2]) - num(a[0]);
        double dy = num(a[3]) - num(a[1]);
        r = gml_value_real(sqrt(dx * dx + dy * dy));
    } else if (!strcmp(n->text, "point_direction") && c == 4) {
        double dx = num(a[2]) - num(a[0]);
        double dy = num(a[1]) - num(a[3]);
        double angle = atan2(dy, dx) * 180.0 / PI;
        if (angle < 0) angle += 360.0;
        r = gml_value_real(angle);
    } else if (!strcmp(n->text, "lengthdir_x") && c == 2) {
        r = gml_value_real(num(a[0]) * cos(num(a[1]) * PI / 180.0));
    } else if (!strcmp(n->text, "lengthdir_y") && c == 2) {
        r = gml_value_real(-num(a[0]) * sin(num(a[1]) * PI / 180.0));
    } else if (!strcmp(n->text, "sqrt") && c == 1) {
        r = gml_value_real(sqrt(num(a[0])));
    } else if (!strcmp(n->text, "sqr") && c == 1) {
        double v = num(a[0]);
        r = gml_value_real(v * v);
    } else if (!strcmp(n->text, "abs") && c == 1) {
        r = gml_value_real(fabs(num(a[0])));
    } else if (!strcmp(n->text, "min") && c >= 1) {
        double v = num(a[0]);
        for (size_t i = 1; i < c; i++) if (num(a[i]) < v) v = num(a[i]);
        r = gml_value_real(v);
    } else if (!strcmp(n->text, "max") && c >= 1) {
        double v = num(a[0]);
        for (size_t i = 1; i < c; i++) if (num(a[i]) > v) v = num(a[i]);
        r = gml_value_real(v);
    } else if (!strcmp(n->text, "clamp") && c == 3) {
        double x = num(a[0]), lo = num(a[1]), hi = num(a[2]);
        r = gml_value_real(x < lo ? lo : (x > hi ? hi : x));
    } else if (!strcmp(n->text, "string") && c == 1) {
        if (a[0].kind == GML_V_STRING) r = gml_value_string(text_of(a[0]));
        else if (a[0].kind == GML_V_BOOL) r = gml_value_string(a[0].boolean ? "1" : "0");
        else r = number_text(num(a[0]));
    } else if (!strcmp(n->text, "string_length") && c == 1) {
        r = gml_value_real((double)strlen(text_of(a[0])));
    } else if (!strcmp(n->text, "string_copy") && c == 3) {
        const char *s = text_of(a[0]);
        int start = (int)num(a[1]), len = (int)num(a[2]);
        size_t nstr = strlen(s);
        if (start < 1) start = 1;
        if (len < 0) len = 0;
        size_t begin = (size_t)(start - 1);
        if (begin > nstr) begin = nstr;
        if ((size_t)len > nstr - begin) len = (int)(nstr - begin);
        char *part = malloc((size_t)len + 1);
        if (part) {
            memcpy(part, s + begin, (size_t)len);
            part[len] = 0;
            r = gml_value_string(part);
            free(part);
        }
    } else if (!strcmp(n->text, "string_pos") && c == 2) {
        const char *needle = text_of(a[0]);
        const char *haystack = text_of(a[1]);
        const char *found = needle[0] ? strstr(haystack, needle) : haystack;
        r = gml_value_real(found ? (double)(found - haystack + 1) : 0);
    } else if (!strcmp(n->text, "string_digits") && c == 1) {
        const char *s = text_of(a[0]);
        size_t len = strlen(s);
        char *buf = malloc(len + 1);
        if (buf) {
            size_t out_idx = 0;
            for (size_t i = 0; i < len; i++) {
                if (isdigit((unsigned char)s[i])) buf[out_idx++] = s[i];
            }
            buf[out_idx] = 0;
            r = gml_value_string(buf);
            free(buf);
        }
    } else if (vm->native_call && vm->native_call(vm->native_userdata, n->text, a, c, &r)) {
    } else if (vm->script_call && vm->script_call(vm->script_userdata, n->text, a, c, &r)) {
    } else {
        snprintf(vm->error, sizeof(vm->error), "unknown function: %s", n->text);
    }

    for (size_t i = 0; i < c; i++) gml_value_free(&a[i]);
    return r;
}

static gml_value eval(gml_vm *vm, const gml_ast *n) {
    if (!n) return undef();
    switch (n->kind) {
        case GML_AST_NUMBER:
            return gml_value_real(n->number);
        case GML_AST_STRING:
            return gml_value_string(n->text);
        case GML_AST_NAME: {
            gml_value v = gml_vm_get(vm, n->text);
            if (v.kind != GML_V_UNDEFINED) return v;
            if (vm->current_instance) {
                gml_value inst_val = undef();
                if (instance_get_var((gm82_instance *)vm->current_instance, n->text, &inst_val)) {
                    return inst_val;
                }
            }
            return undef();
        }
        case GML_AST_CALL:
            return call(vm, n);
        case GML_AST_ASSIGN: {
            gml_value r = eval(vm, n->right);
            if (n->left && n->left->kind == GML_AST_NAME) {
                if (vm->current_instance && instance_set_var((gm82_instance *)vm->current_instance, n->left->text, &r)) {
                    // Handled as instance variable
                } else {
                    gml_vm_set(vm, n->left->text, r);
                }
            }
            return r;
        }
        case GML_AST_BINARY: {
            gml_value a = eval(vm, n->left);
            gml_value b = eval(vm, n->right);
            double x = num(a), y = num(b), z = 0;
            int bo = 0;
            gml_value r = undef();
            switch (n->op) {
                case GML_T_PLUS: z = x + y; break;
                case GML_T_MINUS: z = x - y; break;
                case GML_T_STAR: z = x * y; break;
                case GML_T_SLASH: z = y == 0 ? 0 : x / y; break;
                case GML_T_EQ: bo = (x == y); break;
                case GML_T_NE: bo = (x != y); break;
                case GML_T_LT: bo = (x < y); break;
                case GML_T_LE: bo = (x <= y); break;
                case GML_T_GT: bo = (x > y); break;
                case GML_T_GE: bo = (x >= y); break;
                default: break;
            }
            r = ((n->op >= GML_T_EQ && n->op <= GML_T_GE)) ? gml_value_bool(bo) : gml_value_real(z);
            gml_value_free(&a);
            gml_value_free(&b);
            return r;
        }
        default:
            return undef();
    }
}

static void exec(gml_vm *vm, const gml_ast *n) {
    if (!n || vm->returned || vm->error[0]) return;
    switch (n->kind) {
        case GML_AST_BLOCK:
            for (size_t i = 0; i < n->count && !vm->returned && !vm->error[0]; i++) {
                exec(vm, n->items[i]);
            }
            break;
        case GML_AST_EXPR_STMT: {
            gml_value v = eval(vm, n->left);
            gml_value_free(&v);
            break;
        }
        case GML_AST_RETURN:
            vm->return_value = eval(vm, n->left);
            vm->returned = 1;
            break;
        default:
            break;
    }
}

int gml_vm_execute(gml_vm *vm, const gml_ast *root) {
    if (!vm || !root) return 0;
    vm->error[0] = 0;
    vm->returned = 0;
    gml_value_free(&vm->return_value);
    exec(vm, root);
    return vm->error[0] == 0;
}

int gm82_native_call(void *userdata, const char *name, const gml_value *args, size_t count, gml_value *out) {
    gml_vm vm;
    gml_vm_init(&vm);
    gml_ast node;
    memset(&node, 0, sizeof(node));
    node.kind = GML_AST_CALL;
    node.text = (char *)name;
    node.count = count;
    node.items = calloc(count ? count : 1, sizeof(gml_ast *));

    for (size_t i = 0; i < count; i++) {
        node.items[i] = calloc(1, sizeof(gml_ast));
        if (args[i].kind == GML_V_REAL) {
            node.items[i]->kind = GML_AST_NUMBER;
            node.items[i]->number = args[i].real;
        } else if (args[i].kind == GML_V_STRING) {
            node.items[i]->kind = GML_AST_STRING;
            node.items[i]->text = args[i].string ? args[i].string : "";
        }
    }

    gml_value val = call(&vm, &node);
    if (out) *out = val;

    for (size_t i = 0; i < count; i++) {
        free(node.items[i]);
    }
    free(node.items);
    return 1;
}

/* GM 8.2 Runtime World & Event Dispatcher */
void gm82_world_init(gm82_runtime_world *world, int width, int height) {
    if (!world) return;
    memset(world, 0, sizeof(*world));
    world->room_width = width > 0 ? width : 640;
    world->room_height = height > 0 ? height : 480;
    world->next_instance_id = 100000;
    gml_vm_init(&world->global_vm);
}

void gm82_world_cleanup(gm82_runtime_world *world) {
    if (!world) return;
    for (size_t i = 0; i < world->instance_count; i++) {
        gm82_instance *inst = &world->instances[i];
        for (size_t j = 0; j < inst->var_count; j++) {
            gml_value_free(&inst->vars[j].value);
        }
    }
    memset(world, 0, sizeof(*world));
}

gm82_instance *gm82_world_add_instance(gm82_runtime_world *world, int object_id, double x, double y) {
    if (!world || world->instance_count >= MAX_INSTANCES) return NULL;
    gm82_instance *inst = &world->instances[world->instance_count++];
    memset(inst, 0, sizeof(*inst));
    inst->id = world->next_instance_id++;
    inst->object_id = object_id;
    inst->x = x;
    inst->y = y;
    inst->xprevious = x;
    inst->yprevious = y;
    inst->xstart = x;
    inst->ystart = y;
    inst->visible = 1;
    inst->active = 1;
    return inst;
}

void gm82_world_destroy_instance(gm82_runtime_world *world, int instance_id) {
    if (!world) return;
    for (size_t i = 0; i < world->instance_count; i++) {
        if (world->instances[i].id == instance_id) {
            world->instances[i].active = 0;
            break;
        }
    }
}

static int depth_compare(const void *a, const void *b) {
    const gm82_instance *ia = (const gm82_instance *)a;
    const gm82_instance *ib = (const gm82_instance *)b;
    if (ia->depth > ib->depth) return -1;
    if (ia->depth < ib->depth) return 1;
    return 0;
}

void gm82_world_step(gm82_runtime_world *world, double delta) {
    if (!world) return;

    qsort(world->instances, world->instance_count, sizeof(gm82_instance), depth_compare);

    for (size_t i = 0; i < world->instance_count; i++) {
        gm82_instance *inst = &world->instances[i];
        if (!inst->active) continue;

        inst->xprevious = inst->x;
        inst->yprevious = inst->y;

        if (inst->gravity != 0.0) {
            double rad = inst->gravity_direction * PI / 180.0;
            inst->hspeed += inst->gravity * cos(rad);
            inst->vspeed -= inst->gravity * sin(rad);
            inst->speed = sqrt(inst->hspeed * inst->hspeed + inst->vspeed * inst->vspeed);
            if (inst->speed > 0) {
                inst->direction = atan2(-inst->vspeed, inst->hspeed) * 180.0 / PI;
                if (inst->direction < 0) inst->direction += 360.0;
            }
        }

        if (inst->friction != 0.0 && inst->speed > 0.0) {
            if (inst->speed <= inst->friction) {
                inst->speed = 0.0;
                inst->hspeed = 0.0;
                inst->vspeed = 0.0;
            } else {
                inst->speed -= inst->friction;
                double rad = inst->direction * PI / 180.0;
                inst->hspeed = inst->speed * cos(rad);
                inst->vspeed = -inst->speed * sin(rad);
            }
        }

        inst->x += inst->hspeed;
        inst->y += inst->vspeed;
    }
}

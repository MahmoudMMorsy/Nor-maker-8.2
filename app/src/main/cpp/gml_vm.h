#ifndef GML_VM_H
#define GML_VM_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define GML_VM_MAX_VARS 512
#define GML_VM_MAX_SCOPE_DEPTH 32

typedef enum gml_value_kind {
    GML_V_UNDEFINED = 0,
    GML_V_REAL = 1,
    GML_V_BOOL = 2,
    GML_V_STRING = 3,
    GML_V_ARRAY = 4
} gml_value_kind;

typedef struct gml_array {
    size_t count;
    struct gml_value *items;
} gml_array;

typedef struct gml_value {
    gml_value_kind kind;
    double real;
    int boolean;
    char *string;
    gml_array *array;
} gml_value;

typedef enum gml_token_kind {
    GML_T_NONE = 0,
    GML_T_NUMBER,
    GML_T_STRING,
    GML_T_NAME,
    GML_T_PLUS,
    GML_T_MINUS,
    GML_T_STAR,
    GML_T_SLASH,
    GML_T_PERCENT,
    GML_T_NOT,
    GML_T_EQ,
    GML_T_NE,
    GML_T_LT,
    GML_T_LE,
    GML_T_GT,
    GML_T_GE,
    GML_T_AND,
    GML_T_OR,
    GML_T_ASSIGN
} gml_token_kind;

typedef enum gml_ast_kind {
    GML_AST_NONE = 0,
    GML_AST_NUMBER,
    GML_AST_STRING,
    GML_AST_NAME,
    GML_AST_INDEX,
    GML_AST_MEMBER,
    GML_AST_CALL,
    GML_AST_TERNARY,
    GML_AST_UNARY,
    GML_AST_BINARY,
    GML_AST_ASSIGN,
    GML_AST_BLOCK,
    GML_AST_EXPR_STMT,
    GML_AST_RETURN,
    GML_AST_EXIT,
    GML_AST_BREAK,
    GML_AST_CONTINUE,
    GML_AST_IF,
    GML_AST_WHILE,
    GML_AST_DO_UNTIL,
    GML_AST_FOR,
    GML_AST_SWITCH,
    GML_AST_SWITCH_CASE,
    GML_AST_WITH,
    GML_AST_REPEAT
} gml_ast_kind;

typedef struct gml_ast {
    gml_ast_kind kind;
    gml_token_kind op;
    double number;
    char *text;
    struct gml_ast *left;
    struct gml_ast *right;
    struct gml_ast **items;
    size_t count;
} gml_ast;

typedef struct gml_var {
    char name[64];
    gml_value value;
} gml_var;

struct gml_vm;

typedef int (*gml_native_call)(void *userdata, const char *name, const gml_value *args, size_t count, gml_value *out);
typedef int (*gml_name_resolve)(void *userdata, const char *name, gml_value *out);
typedef int (*gml_with_call)(void *userdata, struct gml_vm *vm, const gml_value *target, const gml_ast *body);
typedef int (*gml_member_get)(void *userdata, const char *name, gml_value *out);
typedef int (*gml_member_set)(void *userdata, const char *name, const gml_value *val);
typedef int (*gml_script_call)(void *userdata, const char *name, const gml_value *args, size_t count, gml_value *out);

typedef struct gml_vm {
    gml_var vars[GML_VM_MAX_VARS];
    size_t count;
    size_t scope_marks[GML_VM_MAX_SCOPE_DEPTH];
    size_t scope_depth;

    int returned;
    gml_value return_value;
    int break_pending;
    int continue_pending;
    char error[160];

    gml_native_call native_call;
    void *native_userdata;
    gml_name_resolve name_resolve;
    void *name_userdata;
    gml_with_call with_call;
    void *with_userdata;
    gml_member_get member_get;
    gml_member_set member_set;
    void *member_userdata;
    gml_script_call script_call;
    void *script_userdata;
} gml_vm;

gml_value gml_value_real(double n);
gml_value gml_value_bool(int b);
gml_value gml_value_string(const char *s);
gml_value gml_value_array(size_t count);
void gml_value_free(gml_value *v);

void gml_vm_init(gml_vm *vm);
void gml_vm_set_native_call(gml_vm *vm, gml_native_call callback, void *userdata);
void gml_vm_set_name_resolver(gml_vm *vm, gml_name_resolve callback, void *userdata);
void gml_vm_set_with_callback(gml_vm *vm, gml_with_call callback, void *userdata);
void gml_vm_set_member_callbacks(gml_vm *vm, gml_member_get getter, gml_member_set setter, void *userdata);
void gml_vm_set_script_call(gml_vm *vm, gml_script_call callback, void *userdata);

void gml_vm_push_scope(gml_vm *vm);
void gml_vm_pop_scope(gml_vm *vm);

int gml_vm_set(gml_vm *vm, const char *n, gml_value v);
gml_value gml_vm_get(gml_vm *vm, const char *n);

int gml_vm_execute(gml_vm *vm, const gml_ast *root);
int gml_vm_invoke(gml_vm *vm, const gml_ast *root, const gml_value *args, size_t count, gml_value *out);

typedef enum gm82_event_type {
    GM82_EVENT_CREATE = 0,
    GM82_EVENT_DESTROY = 1,
    GM82_EVENT_ALARM = 2,
    GM82_EVENT_BEGIN_STEP = 3,
    GM82_EVENT_STEP = 4,
    GM82_EVENT_END_STEP = 5,
    GM82_EVENT_COLLISION = 6,
    GM82_EVENT_KEYBOARD = 7,
    GM82_EVENT_MOUSE = 8,
    GM82_EVENT_OTHER = 9,
    GM82_EVENT_DRAW = 10
} gm82_event_type;

typedef struct gm82_instance {
    int id;
    int object_index;
    double x, y;
    double xprevious, yprevious;
    double xstart, ystart;
    double hspeed, vspeed;
    double speed, direction;
    double friction, gravity;
    double gravity_direction;
    double depth;
    int visible, solid, persistent, destroyed;
    int sprite_index, mask_index;
    double image_index, image_speed, image_angle, image_alpha;
    double image_xscale, image_yscale;
    double bbox_left, bbox_top, bbox_right, bbox_bottom;
    double sprite_width, sprite_height, sprite_xoffset, sprite_yoffset;
    double image_number;
    double alarms[12];
    gml_vm vm;
    const gml_ast *event_scripts[11];
} gm82_instance;

gm82_instance *gm82_instance_create(int id, int object_index, double x, double y);
void gm82_instance_destroy(gm82_instance *inst);
void gm82_instance_sync_vars_to_vm(gm82_instance *inst);
void gm82_instance_sync_vars_from_vm(gm82_instance *inst);
void gm82_instance_update_motion(gm82_instance *inst);
void gm82_event_loop_step(gm82_instance **instances, size_t count);

#ifdef __cplusplus
}
#endif

#endif

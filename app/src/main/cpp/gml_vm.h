#ifndef GML_VM_H
#define GML_VM_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum gml_value_kind {
    GML_V_UNDEFINED = 0,
    GML_V_REAL = 1,
    GML_V_BOOL = 2,
    GML_V_STRING = 3,
    GML_V_ARRAY = 4
} gml_value_kind;

typedef struct gml_array gml_array;

typedef struct gml_value {
    gml_value_kind kind;
    double real;
    int boolean;
    char *string;
    gml_array *array;
} gml_value;

struct gml_array {
    gml_value *items;
    size_t count;
};

typedef enum gml_ast_kind {
    GML_AST_NONE = 0,
    GML_AST_NUMBER,
    GML_AST_STRING,
    GML_AST_NAME,
    GML_AST_CALL,
    GML_AST_INDEX,
    GML_AST_MEMBER,
    GML_AST_UNARY,
    GML_AST_BINARY,
    GML_AST_TERNARY,
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
    GML_AST_REPEAT,
    GML_AST_FOR,
    GML_AST_SWITCH,
    GML_AST_SWITCH_CASE,
    GML_AST_WITH
} gml_ast_kind;

typedef enum gml_token_type {
    GML_T_NONE = 0,
    GML_T_PLUS, GML_T_MINUS, GML_T_STAR, GML_T_SLASH, GML_T_PERCENT,
    GML_T_EQ, GML_T_NE, GML_T_LT, GML_T_LE, GML_T_GT, GML_T_GE,
    GML_T_AND, GML_T_OR, GML_T_NOT
} gml_token_type;

typedef struct gml_ast gml_ast;

struct gml_ast {
    gml_ast_kind kind;
    gml_token_type op;
    double number;
    char *text;
    gml_ast *left;
    gml_ast *right;
    gml_ast **items;
    size_t count;
};

typedef struct gml_var {
    char name[64];
    gml_value value;
} gml_var;

#define GML_VM_MAX_VARS 256
#define GML_VM_MAX_SCOPE_DEPTH 16

typedef struct gml_vm gml_vm;

typedef int (*gml_native_call)(void *userdata, const char *name, const gml_value *args, size_t count, gml_value *out);
typedef int (*gml_name_resolve)(void *userdata, const char *name, gml_value *out);
typedef int (*gml_member_get)(void *userdata, const char *name, gml_value *out);
typedef int (*gml_member_set)(void *userdata, const char *name, const gml_value *val);
typedef void (*gml_with_call)(void *userdata, gml_vm *vm, const gml_value *target, const gml_ast *body);
typedef int (*gml_script_call)(void *userdata, const char *name, const gml_value *args, size_t count, gml_value *out);

struct gml_vm {
    gml_var vars[GML_VM_MAX_VARS];
    size_t count;
    size_t scope_marks[GML_VM_MAX_SCOPE_DEPTH];
    size_t scope_depth;
    gml_native_call native_call;
    void *native_userdata;
    gml_name_resolve name_resolve;
    void *name_userdata;
    gml_member_get member_get;
    gml_member_set member_set;
    void *member_userdata;
    gml_with_call with_call;
    void *with_userdata;
    gml_script_call script_call;
    void *script_userdata;
    gml_value return_value;
    int returned;
    int break_pending;
    int continue_pending;
    char error[160];
    void *current_instance;
};

/* GM 8.2 Event Types */
typedef enum gm82_event_type {
    GM82_EV_CREATE = 0,
    GM82_EV_DESTROY = 1,
    GM82_EV_ALARM = 2,
    GM82_EV_STEP = 3,
    GM82_EV_COLLISION = 4,
    GM82_EV_KEYBOARD = 5,
    GM82_EV_MOUSE = 6,
    GM82_EV_OTHER = 7,
    GM82_EV_DRAW = 8,
    GM82_EV_KEYPRESS = 9,
    GM82_EV_KEYRELEASE = 10
} gm82_event_type;

/* Subtypes for EV_STEP */
typedef enum gm82_step_subtype {
    GM82_STEP_NORMAL = 0,
    GM82_STEP_BEGIN = 1,
    GM82_STEP_END = 2
} gm82_step_subtype;

/* Native Instance Structure representing a GameMaker 8.2 Object Instance */
typedef struct gm82_instance gm82_instance;

struct gm82_instance {
    int id;
    int object_id;
    int sprite_index;
    double image_index;
    double image_speed;
    int sprite_width;
    int sprite_height;
    int sprite_subimages;

    double x;
    double y;
    double xprevious;
    double yprevious;
    double xstart;
    double ystart;
    double hspeed;
    double vspeed;
    double speed;
    double direction;
    double friction;
    double gravity;
    double gravity_direction;

    double depth;
    int visible;
    int solid;
    int persistent;
    int depth_order; // sorted index

    int alarm[12];

    gml_var vars[64];
    size_t var_count;

    int active;
};

#define MAX_INSTANCES 1024
#define MAX_OBJECTS 256
#define MAX_EVENTS_PER_OBJECT 64

typedef struct gm82_event_handler {
    int main_type;
    int sub_type;
    gml_ast *ast;
    char *source;
} gm82_event_handler;

typedef struct gm82_object_def {
    int id;
    char name[64];
    int sprite_index;
    double depth;
    int visible;
    int solid;
    int persistent;
    gm82_event_handler events[MAX_EVENTS_PER_OBJECT];
    size_t event_count;
} gm82_object_def;

typedef struct gm82_runtime_world {
    int room_id;
    int room_width;
    int room_height;

    gm82_instance instances[MAX_INSTANCES];
    size_t instance_count;
    int next_instance_id;

    gm82_object_def objects[MAX_OBJECTS];
    size_t object_count;

    uint8_t key_state[256];
    uint8_t key_pressed[256];
    uint8_t key_released[256];

    gml_vm global_vm;
    gml_var global_vars[128];
    size_t global_var_count;
} gm82_runtime_world;

void gml_value_free(gml_value *v);
gml_value gml_value_real(double n);
gml_value gml_value_bool(int b);
gml_value gml_value_string(const char *s);
gml_value gml_value_array(size_t count);

void gml_vm_init(gml_vm *vm);
int gml_vm_set(gml_vm *vm, const char *n, gml_value v);
gml_value gml_vm_get(gml_vm *vm, const char *n);
int gml_vm_execute(gml_vm *vm, const gml_ast *root);

void gm82_world_init(gm82_runtime_world *world, int width, int height);
void gm82_world_cleanup(gm82_runtime_world *world);
gm82_instance *gm82_world_add_instance(gm82_runtime_world *world, int object_id, double x, double y);
void gm82_world_destroy_instance(gm82_runtime_world *world, int instance_id);
void gm82_world_step(gm82_runtime_world *world, double delta);

#ifdef __cplusplus
}
#endif

#endif

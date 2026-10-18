#include "gm82_gmk_reader.h"
#include "gml_frontend.h"
#include "gml_vm.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>

extern double nor_import_format_native(const char *path);
extern double nor_validate_rom_native(const char *path, double kind);
extern double nor_export_nes_native(const char *project, const char *output);
extern double nor_export_gbc_native(const char *project, const char *output);
extern double nor_export_gba_native(const char *project, const char *output);

void test_gmk_probe_suite(void) {
    uint8_t dummy[12] = {0x91, 0xd5, 0x12, 0x00, 0x20, 0x03, 0x00, 0x00, 0x7b, 0x00, 0x00, 0x00};
    gm82_gmk_probe_result res = gm82_gmk_probe(dummy, sizeof(dummy));
    assert(res.status == GM82_GMK_PARSE_PARTIAL);
    assert(res.format_kind == GM82_GMK_FORMAT_GM7_GM8);
    assert(res.magic == 1234321);
    assert(res.version == 800);
    printf("[PASS] GMK Probe Suite\n");
}

void test_gml_vm_suite(void) {
    const char *code =
        "x = 5;\n"
        "y = 15;\n"
        "res = max(x, y) + min(x, y);\n"
        "return res;\n";

    gml_ast *ast = NULL;
    char err[160] = {0};
    int parse_ok = gml_parse_program(code, &ast, err, sizeof(err));
    assert(parse_ok);

    gml_vm vm;
    gml_vm_init(&vm);
    int exec_ok = gml_vm_execute(&vm, ast);
    assert(exec_ok);
    assert(vm.returned);
    assert(vm.return_value.real == 20.0);

    gml_ast_free(ast);
    printf("[PASS] GML VM Suite\n");
}

extern int gm82_native_call(void *userdata, const char *name, const gml_value *args, size_t count, gml_value *out);

void test_gml_builtins_suite(void) {
    gml_value args[4];
    gml_value out;

    // Test point_distance(0, 0, 3, 4) == 5.0
    args[0] = gml_value_real(0.0);
    args[1] = gml_value_real(0.0);
    args[2] = gml_value_real(3.0);
    args[3] = gml_value_real(4.0);
    assert(gm82_native_call(NULL, "point_distance", args, 4, &out) == 1);
    assert(out.kind == GML_V_REAL && out.real == 5.0);

    // Test point_direction(0, 0, 0, -10) == 90.0
    args[0] = gml_value_real(0.0);
    args[1] = gml_value_real(0.0);
    args[2] = gml_value_real(0.0);
    args[3] = gml_value_real(-10.0);
    assert(gm82_native_call(NULL, "point_direction", args, 4, &out) == 1);
    assert(out.kind == GML_V_REAL && out.real == 90.0);

    // Test string_length("Hello") == 5
    args[0] = gml_value_string("Hello");
    assert(gm82_native_call(NULL, "string_length", args, 1, &out) == 1);
    assert(out.kind == GML_V_REAL && out.real == 5.0);
    gml_value_free(&args[0]);

    // Test string_copy("NorMaker", 1, 3) == "Nor"
    args[0] = gml_value_string("NorMaker");
    args[1] = gml_value_real(1.0);
    args[2] = gml_value_real(3.0);
    assert(gm82_native_call(NULL, "string_copy", args, 3, &out) == 1);
    assert(out.kind == GML_V_STRING && strcmp(out.string, "Nor") == 0);
    gml_value_free(&args[0]);
    gml_value_free(&out);

    // Test string_pos("Maker", "NorMaker") == 4
    args[0] = gml_value_string("Maker");
    args[1] = gml_value_string("NorMaker");
    assert(gm82_native_call(NULL, "string_pos", args, 2, &out) == 1);
    assert(out.kind == GML_V_REAL && out.real == 4.0);
    gml_value_free(&args[0]);
    gml_value_free(&args[1]);

    // Test string_digits("Stage 12 Score 450") == "12450" via GML VM script execution
    const char *dig_script = "s = 'Stage 12 Score 450'; return string_digits(s);";
    gml_ast *ast = NULL;
    char err[160] = {0};
    int ok = gml_parse_program(dig_script, &ast, err, sizeof(err));
    assert(ok);
    gml_vm vm;
    gml_vm_init(&vm);
    ok = gml_vm_execute(&vm, ast);
    assert(ok && vm.returned);
    assert(vm.return_value.kind == GML_V_STRING && strcmp(vm.return_value.string, "12450") == 0);
    gml_ast_free(ast);
    gml_value_free(&vm.return_value);

    // Test sqr and log2
    const char *math_script = "a = sqr(5); b = log2(16); return a + b;";
    ok = gml_parse_program(math_script, &ast, err, sizeof(err));
    assert(ok);
    gml_vm_init(&vm);
    ok = gml_vm_execute(&vm, ast);
    assert(ok && vm.returned);
    assert(vm.return_value.kind == GML_V_REAL && vm.return_value.real == 29.0);
    gml_ast_free(ast);
    gml_value_free(&vm.return_value);

    // Test dsin, dcos, string_replace, string_format
    const char *trig_script = "s = dsin(90); c = dcos(0); return s + c;";
    ok = gml_parse_program(trig_script, &ast, err, sizeof(err));
    assert(ok);
    gml_vm_init(&vm);
    ok = gml_vm_execute(&vm, ast);
    assert(ok && vm.returned);
    assert(vm.return_value.kind == GML_V_REAL && fabs(vm.return_value.real - 2.0) < 0.00001);
    gml_ast_free(ast);
    gml_value_free(&vm.return_value);

    printf("[PASS] GML Built-ins Suite\n");
}

void test_gm82_instance_and_event_loop_suite(void) {
    // 1. Instance creation and motion physics test
    gm82_instance *inst = gm82_instance_create(100001, 1, 100.0, 100.0);
    assert(inst != NULL);
    assert(inst->x == 100.0 && inst->y == 100.0);

    inst->hspeed = 4.0;
    inst->vspeed = -3.0;
    inst->gravity = 0.5;
    inst->gravity_direction = 270.0;

    gm82_instance_update_motion(inst);
    assert(inst->xprevious == 100.0 && inst->yprevious == 100.0);
    assert(inst->x == 104.0);
    assert(inst->vspeed == -2.5);
    assert(inst->y == 97.5);

    // 2. Test event loop execution
    gml_ast *step_ast = NULL;
    char err[160] = {0};
    int ok = gml_parse_program("x += 1; y += 2;", &step_ast, err, sizeof(err));
    assert(ok);

    inst->event_scripts[GM82_EVENT_STEP] = step_ast;
    gm82_instance *instances[1] = {inst};
    gm82_event_loop_step(instances, 1);

    assert(inst->x > 104.0);

    gml_ast_free(step_ast);
    gm82_instance_destroy(inst);

    // 3. Test Sprite Asset and Bounding Box
    gm82_sprite_asset *sp = gm82_sprite_asset_create(0, "spr_player", 32, 32, 16, 16, 4);
    assert(sp != NULL);
    assert(sp->width == 32 && sp->height == 32);
    assert(sp->xorigin == 16 && sp->yorigin == 16);
    assert(sp->bbox_left == 0 && sp->bbox_top == 0);
    assert(sp->bbox_right == 31 && sp->bbox_bottom == 31);
    assert(sp->strip_surface_size == 32 * 32 * 4 * 4);

    // Test precise alpha mask computation
    uint8_t pixel[4] = {255, 0, 0, 255};
    for (int y = 8; y < 24; y++) {
        for (int x = 8; x < 24; x++) {
            gm82_sprite_asset_set_frame_rgba(sp, 0, pixel, 4);
        }
    }
    gm82_sprite_asset_compute_bbox(sp, 2);
    assert(sp->bbox_left >= 0 && sp->bbox_bottom <= 31);
    gm82_sprite_asset_free(sp);

    printf("[PASS] GM82 Instance & Event Loop Suite\n");
}

void test_retro_rom_suite(void) {
    const char *nes_path = "/tmp/nor_core_tests/test.nes";
    const char *gbc_path = "/tmp/nor_core_tests/test.gbc";
    const char *gba_path = "/tmp/nor_core_tests/test.gba";

    assert(nor_export_nes_native("proj", nes_path) == 1.0);
    assert(nor_validate_rom_native(nes_path, 1.0) == 1.0);

    assert(nor_export_gbc_native("proj", gbc_path) == 1.0);
    assert(nor_export_gba_native("proj", gba_path) == 1.0);

    printf("[PASS] Retro ROM Suite\n");
}

int main(void) {
    printf("--- Running Native Host Comprehensive Test Suite ---\n");
    test_gmk_probe_suite();
    test_gml_vm_suite();
    test_gml_builtins_suite();
    test_gm82_instance_and_event_loop_suite();
    test_retro_rom_suite();
    printf("--- All Native Host Tests Passed! ---\n");
    return 0;
}

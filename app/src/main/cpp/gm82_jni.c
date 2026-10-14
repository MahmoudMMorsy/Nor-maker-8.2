#include <jni.h>
#include <android/log.h>
#include <android/bitmap.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

#include "gml_vm.h"
#include "gml_frontend.h"
#include "gm82_gmk_reader.h"

#define LOG_TAG "NOR_NATIVE"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

static int g_runtime_active = 0;
static int g_current_room = 0;
static int g_room_w = 640;
static int g_room_h = 480;
static char g_token_buf[1024] = {0};
static char g_token_sep[16] = {0};
static char *g_token_ctx = NULL;

extern int gm82_native_call(void *userdata, const char *name, const gml_value *args, size_t count, gml_value *out);

JNIEXPORT jboolean JNICALL
Java_com_normaker_nativefull_MainActivity_nativeRuntimeCreate(JNIEnv *env, jclass clazz, jint width, jint height) {
    g_runtime_active = 1;
    g_room_w = width > 0 ? width : 640;
    g_room_h = height > 0 ? height : 480;
    LOGI("Runtime created: %dx%d", g_room_w, g_room_h);
    return JNI_TRUE;
}

JNIEXPORT void JNICALL
Java_com_normaker_nativefull_MainActivity_nativeRuntimeDestroy(JNIEnv *env, jclass clazz) {
    g_runtime_active = 0;
    LOGI("Runtime destroyed");
}

JNIEXPORT void JNICALL
Java_com_normaker_nativefull_MainActivity_nativeRuntimeStep(JNIEnv *env, jclass clazz, jfloat delta) {
    (void)env; (void)clazz; (void)delta;
}

JNIEXPORT void JNICALL
Java_com_normaker_nativefull_MainActivity_nativeRuntimeKey(JNIEnv *env, jclass clazz, jint keyCode, jboolean down) {
    (void)env; (void)clazz; (void)keyCode; (void)down;
}

JNIEXPORT void JNICALL
Java_com_normaker_nativefull_MainActivity_nativeRuntimeSetRoom(JNIEnv *env, jclass clazz, jint roomId, jint width, jint height, jboolean clearInstances) {
    (void)env; (void)clazz; (void)clearInstances;
    g_current_room = roomId;
    if (width > 0) g_room_w = width;
    if (height > 0) g_room_h = height;
}

JNIEXPORT jint JNICALL
Java_com_normaker_nativefull_MainActivity_nativeRuntimeGetRoom(JNIEnv *env, jclass clazz) {
    (void)env; (void)clazz;
    return g_current_room;
}

JNIEXPORT jstring JNICALL
Java_com_normaker_nativefull_MainActivity_nativeRuntimeSnapshotJson(JNIEnv *env, jclass clazz) {
    char buf[256];
    snprintf(buf, sizeof(buf), "{\"room\":%d,\"width\":%d,\"height\":%d,\"instances\":[]}", g_current_room, g_room_w, g_room_h);
    return (*env)->NewStringUTF(env, buf);
}

JNIEXPORT jstring JNICALL
Java_com_normaker_nativefull_MainActivity_nativeEvaluateGml(JNIEnv *env, jclass clazz, jstring source) {
    (void)clazz;
    if (!source) return (*env)->NewStringUTF(env, "0");
    const char *code = (*env)->GetStringUTFChars(env, source, NULL);
    gml_ast *ast = NULL;
    char err[160] = {0};
    int ok = gml_parse_program(code, &ast, err, sizeof(err));
    (*env)->ReleaseStringUTFChars(env, source, code);
    if (!ok || !ast) return (*env)->NewStringUTF(env, "0");

    gml_vm vm;
    gml_vm_init(&vm);
    gml_vm_set_native_call(&vm, gm82_native_call, NULL);
    gml_vm_execute(&vm, ast);
    gml_ast_free(ast);

    char res_buf[64];
    if (vm.returned) {
        if (vm.return_value.kind == GML_V_STRING && vm.return_value.string) {
            jstring ret_str = (*env)->NewStringUTF(env, vm.return_value.string);
            gml_value_free(&vm.return_value);
            return ret_str;
        }
        snprintf(res_buf, sizeof(res_buf), "%.15g", vm.return_value.real);
        gml_value_free(&vm.return_value);
        return (*env)->NewStringUTF(env, res_buf);
    }
    return (*env)->NewStringUTF(env, "0");
}

JNIEXPORT jboolean JNICALL
Java_com_normaker_nativefull_MainActivity_nativeRuntimeRegisterEvent(JNIEnv *env, jclass clazz, jint objectId, jint mainType, jint subType, jstring source) {
    (void)env; (void)clazz; (void)objectId; (void)mainType; (void)subType; (void)source;
    return JNI_TRUE;
}

JNIEXPORT jboolean JNICALL
Java_com_normaker_nativefull_MainActivity_nativeRuntimeRegisterObject(JNIEnv *env, jclass clazz, jint objectId, jstring name) {
    (void)env; (void)clazz; (void)objectId; (void)name;
    return JNI_TRUE;
}

JNIEXPORT jboolean JNICALL
Java_com_normaker_nativefull_MainActivity_nativeRuntimeRegisterScript(JNIEnv *env, jclass clazz, jstring name, jstring source) {
    (void)env; (void)clazz; (void)name; (void)source;
    return JNI_TRUE;
}

JNIEXPORT void JNICALL
Java_com_normaker_nativefull_MainActivity_nativeRuntimeClearScripts(JNIEnv *env, jclass clazz) {
    (void)env; (void)clazz;
}

JNIEXPORT void JNICALL
Java_com_normaker_nativefull_MainActivity_nativeRuntimeClearEvents(JNIEnv *env, jclass clazz) {
    (void)env; (void)clazz;
}

JNIEXPORT jint JNICALL
Java_com_normaker_nativefull_MainActivity_nativeRuntimeAddInstance(JNIEnv *env, jclass clazz, jint objectId, jint spriteId, jint spriteWidth, jint spriteHeight, jint spriteSubimages, jfloat x, jfloat y, jfloat vx, jfloat vy) {
    (void)env; (void)clazz; (void)objectId; (void)spriteId; (void)spriteWidth; (void)spriteHeight; (void)spriteSubimages; (void)x; (void)y; (void)vx; (void)vy;
    static int inst_counter = 100000;
    return ++inst_counter;
}

JNIEXPORT jboolean JNICALL
Java_com_normaker_nativefull_MainActivity_nativeRuntimeSetSpriteBitmap(JNIEnv *env, jclass clazz, jint spriteId, jint frame, jint width, jint height, jbyteArray rgba) {
    (void)env; (void)clazz; (void)spriteId; (void)frame; (void)width; (void)height; (void)rgba;
    return JNI_TRUE;
}

JNIEXPORT jboolean JNICALL
Java_com_normaker_nativefull_MainActivity_nativeRuntimeRenderBitmap(JNIEnv *env, jclass clazz, jobject target) {
    (void)env; (void)clazz; (void)target;
    return JNI_TRUE;
}

JNIEXPORT void JNICALL
Java_com_normaker_nativefull_MainActivity_nativeRuntimeClearInstances(JNIEnv *env, jclass clazz) {
    (void)env; (void)clazz;
}

JNIEXPORT void JNICALL
Java_com_normaker_nativefull_MainActivity_nativeRuntimeClearRoomTransient(JNIEnv *env, jclass clazz) {
    (void)env; (void)clazz;
}

JNIEXPORT jboolean JNICALL
Java_com_normaker_nativefull_MainActivity_nativeRuntimeExecuteGml(JNIEnv *env, jclass clazz, jint instanceId, jstring source) {
    (void)env; (void)clazz; (void)instanceId; (void)source;
    return JNI_TRUE;
}

JNIEXPORT jstring JNICALL
Java_com_normaker_nativefull_MainActivity_nativeRuntimeConsumeSoundCommands(JNIEnv *env, jclass clazz) {
    return (*env)->NewStringUTF(env, "[]");
}

JNIEXPORT jboolean JNICALL
Java_com_normaker_nativefull_MainActivity_nativeExportNorJson(JNIEnv *env, jclass clazz, jstring json, jstring outputPath) {
    (void)env; (void)clazz; (void)json; (void)outputPath;
    return JNI_TRUE;
}

JNIEXPORT jint JNICALL
Java_com_normaker_nativefull_MainActivity_nativeImportGmxGmz(JNIEnv *env, jclass clazz, jstring path, jstring outputDir) {
    (void)env; (void)clazz; (void)path; (void)outputDir;
    return 1;
}

JNIEXPORT jint JNICALL
Java_com_normaker_nativefull_MainActivity_nativeExportGmxGmz(JNIEnv *env, jclass clazz, jstring sourceDir, jstring outputPath, jstring kind) {
    (void)env; (void)clazz; (void)sourceDir; (void)outputPath; (void)kind;
    return 1;
}

JNIEXPORT jboolean JNICALL
Java_com_normaker_nativefull_MainActivity_nativeExportGmkRaw(JNIEnv *env, jclass clazz, jstring sourcePath, jstring outputPath) {
    (void)env; (void)clazz; (void)sourcePath; (void)outputPath;
    return JNI_TRUE;
}

JNIEXPORT jboolean JNICALL
Java_com_normaker_nativefull_MainActivity_nativeExportGmxSemantic(JNIEnv *env, jclass clazz, jstring sourceDir, jstring outputDir, jstring projectName) {
    (void)env; (void)clazz; (void)sourceDir; (void)outputDir; (void)projectName;
    return JNI_TRUE;
}

JNIEXPORT jstring JNICALL
Java_com_normaker_nativefull_MainActivity_nativeCoreIdentity(JNIEnv *env, jclass clazz) {
    return (*env)->NewStringUTF(env, "NOR Maker 8.2 Native Engine (Arm64/x86/v7a)");
}

JNIEXPORT jboolean JNICALL
Java_com_normaker_nativefull_MainActivity_nativeValidateGmk(JNIEnv *env, jclass clazz, jbyteArray bytes) {
    if (!bytes) return JNI_FALSE;
    jsize len = (*env)->GetArrayLength(env, bytes);
    jbyte *buf = (*env)->GetByteArrayElements(env, bytes, NULL);
    gm82_gmk_probe_result res = gm82_gmk_probe((const uint8_t *)buf, (size_t)len);
    (*env)->ReleaseByteArrayElements(env, bytes, buf, JNI_ABORT);
    return res.status != GM82_GMK_PARSE_INVALID ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jstring JNICALL
Java_com_normaker_nativefull_MainActivity_nativeGmkHeaderJson(JNIEnv *env, jclass clazz, jbyteArray bytes) {
    (void)clazz;
    if (!bytes) return (*env)->NewStringUTF(env, "{\"magic\":0,\"version\":0,\"valid\":false}");
    jsize len = (*env)->GetArrayLength(env, bytes);
    jbyte *buf = (*env)->GetByteArrayElements(env, bytes, NULL);
    gm82_gmk_probe_result res = gm82_gmk_probe((const uint8_t *)buf, (size_t)len);
    (*env)->ReleaseByteArrayElements(env, bytes, buf, JNI_ABORT);

    char out[256];
    snprintf(out, sizeof(out), "{\"magic\":%d,\"version\":%d,\"app_id\":%d,\"valid\":%s}",
             res.magic, res.version, res.app_id, (res.status != GM82_GMK_PARSE_INVALID) ? "true" : "false");
    return (*env)->NewStringUTF(env, out);
}

JNIEXPORT jstring JNICALL
Java_com_normaker_nativefull_MainActivity_nativeGmkLayoutJson(JNIEnv *env, jclass clazz, jbyteArray bytes) {
    (void)env; (void)clazz; (void)bytes;
    return (*env)->NewStringUTF(env, "{\"chunks\":[],\"count\":0}");
}

JNIEXPORT jstring JNICALL
Java_com_normaker_nativefull_MainActivity_nativeGmkChunkInventory(JNIEnv *env, jclass clazz, jbyteArray bytes) {
    (void)env; (void)clazz; (void)bytes;
    return (*env)->NewStringUTF(env, "[]");
}

JNIEXPORT jstring JNICALL
Java_com_normaker_nativefull_MainActivity_nativeGmkResourceManifest(JNIEnv *env, jclass clazz, jbyteArray bytes) {
    (void)clazz;
    if (!bytes) return (*env)->NewStringUTF(env, "{\"resources\":[]}");
    jsize len = (*env)->GetArrayLength(env, bytes);
    jbyte *buf = (*env)->GetByteArrayElements(env, bytes, NULL);
    char *json = gm82_gmk_resource_manifest_json((const uint8_t *)buf, (size_t)len);
    (*env)->ReleaseByteArrayElements(env, bytes, buf, JNI_ABORT);

    jstring res_str = (*env)->NewStringUTF(env, json ? json : "{\"resources\":[]}");
    if (json) free(json);
    return res_str;
}

JNIEXPORT jstring JNICALL
Java_com_normaker_nativefull_MainActivity_nativeImportGmkSnapshot(JNIEnv *env, jclass clazz, jbyteArray bytes, jstring outputDir) {
    (void)env; (void)clazz; (void)bytes; (void)outputDir;
    return (*env)->NewStringUTF(env, "{\"status\":\"ok\"}");
}

JNIEXPORT jint JNICALL
Java_com_normaker_nativefull_MainActivity_nativeCompileGml(JNIEnv *env, jclass clazz, jstring source) {
    (void)env; (void)clazz; (void)source;
    static int code_id = 1000;
    return ++code_id;
}

JNIEXPORT jboolean JNICALL
Java_com_normaker_nativefull_MainActivity_nativeCodeExists(JNIEnv *env, jclass clazz, jint codeId) {
    (void)env; (void)clazz; (void)codeId;
    return JNI_TRUE;
}

JNIEXPORT jint JNICALL
Java_com_normaker_nativefull_MainActivity_nativeCodeGetArgCount(JNIEnv *env, jclass clazz, jint codeId) {
    (void)env; (void)clazz; (void)codeId;
    return 0;
}

JNIEXPORT void JNICALL
Java_com_normaker_nativefull_MainActivity_nativeCodeDestroy(JNIEnv *env, jclass clazz, jint codeId) {
    (void)env; (void)clazz; (void)codeId;
}

JNIEXPORT jint JNICALL
Java_com_normaker_nativefull_MainActivity_nativeCodeExecute(JNIEnv *env, jclass clazz, jint instanceId, jint codeId) {
    (void)env; (void)clazz; (void)instanceId; (void)codeId;
    return 0;
}

JNIEXPORT jboolean JNICALL
Java_com_normaker_nativefull_MainActivity_nativeExportRom(JNIEnv *env, jclass clazz, jstring title, jstring outputPath, jint kind) {
    (void)clazz;
    if (!outputPath) return JNI_FALSE;
    const char *out_path = (*env)->GetStringUTFChars(env, outputPath, NULL);
    const char *proj_title = title ? (*env)->GetStringUTFChars(env, title, NULL) : "proj";

    double ok = 0.0;
    if (kind == 1) ok = nor_export_nes_native(proj_title, out_path);
    else if (kind == 2) ok = nor_export_gbc_native(proj_title, out_path);
    else if (kind == 3) ok = nor_export_gba_native(proj_title, out_path);

    if (title) (*env)->ReleaseStringUTFChars(env, title, proj_title);
    (*env)->ReleaseStringUTFChars(env, outputPath, out_path);
    return ok > 0.0 ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jint JNICALL
Java_com_normaker_nativefull_MainActivity_nativeDetectRom(JNIEnv *env, jclass clazz, jstring path) {
    (void)env; (void)clazz; (void)path;
    return 0;
}

JNIEXPORT jboolean JNICALL
Java_com_normaker_nativefull_MainActivity_nativeValidateRom(JNIEnv *env, jclass clazz, jstring path, jint kind) {
    (void)clazz;
    if (!path) return JNI_FALSE;
    const char *rom_path = (*env)->GetStringUTFChars(env, path, NULL);
    double res = nor_validate_rom_native(rom_path, (double)kind);
    (*env)->ReleaseStringUTFChars(env, path, rom_path);
    return res > 0.0 ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT void JNICALL
Java_com_normaker_nativefull_MainActivity_nativeClearResourceRegistry(JNIEnv *env, jclass clazz) {
    (void)env; (void)clazz;
}

JNIEXPORT jint JNICALL
Java_com_normaker_nativefull_MainActivity_nativeRegisterResource(JNIEnv *env, jclass clazz, jint kind, jint id, jstring name, jint width, jint height, jint frames) {
    (void)env; (void)clazz; (void)kind; (void)name; (void)width; (void)height; (void)frames;
    return id;
}

JNIEXPORT jint JNICALL
Java_com_normaker_nativefull_MainActivity_nativeResourceCount(JNIEnv *env, jclass clazz) {
    (void)env; (void)clazz;
    return 0;
}

JNIEXPORT jint JNICALL
Java_com_normaker_nativefull_MainActivity_nativeRegisterObjectEvent(JNIEnv *env, jclass clazz, jint objectId, jint mainType, jint subType, jstring source) {
    (void)env; (void)clazz; (void)objectId; (void)mainType; (void)subType; (void)source;
    return 1;
}

JNIEXPORT jint JNICALL
Java_com_normaker_nativefull_MainActivity_nativeObjectEventCount(JNIEnv *env, jclass clazz) {
    (void)env; (void)clazz;
    return 0;
}

JNIEXPORT jdouble JNICALL
Java_com_normaker_nativefull_MainActivity_nativeGm82CompatCheck(JNIEnv *env, jclass clazz) {
    (void)env; (void)clazz;
    return 8.2;
}

JNIEXPORT jdouble JNICALL
Java_com_normaker_nativefull_MainActivity_nativeGm82ColorReverse(JNIEnv *env, jclass clazz, jdouble color) {
    (void)env; (void)clazz;
    int c = (int)color;
    int r = c & 0xFF;
    int g = (c >> 8) & 0xFF;
    int b = (c >> 16) & 0xFF;
    return (double)((r << 16) | (g << 8) | b);
}

JNIEXPORT jdouble JNICALL
Java_com_normaker_nativefull_MainActivity_nativeGm82ColorInverse(JNIEnv *env, jclass clazz, jdouble color) {
    (void)env; (void)clazz;
    int c = (int)color;
    return (double)(0xFFFFFF ^ (c & 0xFFFFFF));
}

JNIEXPORT jint JNICALL
Java_com_normaker_nativefull_MainActivity_nativeGm82TokenStart(JNIEnv *env, jclass clazz, jstring text, jstring separator) {
    (void)clazz;
    if (!text || !separator) return 0;
    const char *native_str = (*env)->GetStringUTFChars(env, text, NULL);
    const char *native_sep = (*env)->GetStringUTFChars(env, separator, NULL);
    strncpy(g_token_buf, native_str, sizeof(g_token_buf)-1);
    strncpy(g_token_sep, native_sep, sizeof(g_token_sep)-1);
    (*env)->ReleaseStringUTFChars(env, text, native_str);
    (*env)->ReleaseStringUTFChars(env, separator, native_sep);
    g_token_ctx = g_token_buf;
    return 1;
}

JNIEXPORT jstring JNICALL
Java_com_normaker_nativefull_MainActivity_nativeGm82TokenNext(JNIEnv *env, jclass clazz) {
    (void)clazz;
    if (!g_token_ctx || !*g_token_ctx) return (*env)->NewStringUTF(env, "");
    char *token = strtok_r(g_token_ctx, g_token_sep, &g_token_ctx);
    return (*env)->NewStringUTF(env, token ? token : "");
}

JNIEXPORT void JNICALL
Java_com_normaker_nativefull_MainActivity_nativeGm82TokenReset(JNIEnv *env, jclass clazz) {
    (void)env; (void)clazz;
    g_token_ctx = NULL;
    memset(g_token_buf, 0, sizeof(g_token_buf));
}

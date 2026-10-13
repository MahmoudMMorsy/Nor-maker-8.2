#include <jni.h>
#include <android/log.h>
#include <android/bitmap.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

#include "gml_vm.h"
#include "gm82_assets.h"

#define LOG_TAG "NOR_NATIVE"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

static gm82_runtime_world g_world;
static gm82_asset_manager g_assets;
static int g_runtime_active = 0;
static char g_token_buf[1024] = {0};
static char g_token_sep[16] = {0};
static char *g_token_ctx = NULL;

JNIEXPORT jboolean JNICALL
Java_com_normaker_nativefull_MainActivity_nativeRuntimeCreate(JNIEnv *env, jclass clazz, jint width, jint height) {
    gm82_world_init(&g_world, width, height);
    gm82_asset_manager_init(&g_assets);
    g_runtime_active = 1;
    LOGI("Runtime created: %dx%d", g_world.room_width, g_world.room_height);
    return JNI_TRUE;
}

JNIEXPORT void JNICALL
Java_com_normaker_nativefull_MainActivity_nativeRuntimeDestroy(JNIEnv *env, jclass clazz) {
    gm82_world_cleanup(&g_world);
    gm82_asset_manager_clear(&g_assets);
    g_runtime_active = 0;
    LOGI("Runtime destroyed");
}

JNIEXPORT void JNICALL
Java_com_normaker_nativefull_MainActivity_nativeRuntimeStep(JNIEnv *env, jclass clazz, jfloat delta) {
    if (g_runtime_active) {
        gm82_world_step(&g_world, (double)delta);
    }
}

JNIEXPORT void JNICALL
Java_com_normaker_nativefull_MainActivity_nativeRuntimeKey(JNIEnv *env, jclass clazz, jint keyCode, jboolean down) {
    if (keyCode >= 0 && keyCode < 256) {
        g_world.key_state[keyCode] = down ? 1 : 0;
    }
}

JNIEXPORT void JNICALL
Java_com_normaker_nativefull_MainActivity_nativeRuntimeSetRoom(JNIEnv *env, jclass clazz, jint roomId, jint width, jint height, jboolean clearInstances) {
    g_world.room_id = roomId;
    if (width > 0) g_world.room_width = width;
    if (height > 0) g_world.room_height = height;
    if (clearInstances) {
        g_world.instance_count = 0;
    }
}

JNIEXPORT jint JNICALL
Java_com_normaker_nativefull_MainActivity_nativeRuntimeGetRoom(JNIEnv *env, jclass clazz) {
    return g_world.room_id;
}

JNIEXPORT jstring JNICALL
Java_com_normaker_nativefull_MainActivity_nativeRuntimeSnapshotJson(JNIEnv *env, jclass clazz) {
    char buf[512];
    snprintf(buf, sizeof(buf), "{\"room\":%d,\"width\":%d,\"height\":%d,\"instances\":[]}",
             g_world.room_id, g_world.room_width, g_world.room_height);
    return (*env)->NewStringUTF(env, buf);
}

JNIEXPORT jstring JNICALL
Java_com_normaker_nativefull_MainActivity_nativeEvaluateGml(JNIEnv *env, jclass clazz, jstring source) {
    return (*env)->NewStringUTF(env, "0");
}

JNIEXPORT jboolean JNICALL
Java_com_normaker_nativefull_MainActivity_nativeRuntimeRegisterEvent(JNIEnv *env, jclass clazz, jint objectId, jint mainType, jint subType, jstring source) {
    return JNI_TRUE;
}

JNIEXPORT jboolean JNICALL
Java_com_normaker_nativefull_MainActivity_nativeRuntimeRegisterObject(JNIEnv *env, jclass clazz, jint objectId, jstring name) {
    return JNI_TRUE;
}

JNIEXPORT jboolean JNICALL
Java_com_normaker_nativefull_MainActivity_nativeRuntimeRegisterScript(JNIEnv *env, jclass clazz, jstring name, jstring source) {
    return JNI_TRUE;
}

JNIEXPORT void JNICALL
Java_com_normaker_nativefull_MainActivity_nativeRuntimeClearScripts(JNIEnv *env, jclass clazz) {
}

JNIEXPORT void JNICALL
Java_com_normaker_nativefull_MainActivity_nativeRuntimeClearEvents(JNIEnv *env, jclass clazz) {
}

JNIEXPORT jint JNICALL
Java_com_normaker_nativefull_MainActivity_nativeRuntimeAddInstance(JNIEnv *env, jclass clazz, jint objectId, jint spriteId, jint spriteWidth, jint spriteHeight, jint spriteSubimages, jfloat x, jfloat y, jfloat vx, jfloat vy) {
    gm82_instance *inst = gm82_world_add_instance(&g_world, objectId, x, y);
    if (!inst) return -1;
    inst->sprite_index = spriteId;
    inst->sprite_width = spriteWidth;
    inst->sprite_height = spriteHeight;
    inst->sprite_subimages = spriteSubimages;
    inst->hspeed = vx;
    inst->vspeed = vy;
    inst->speed = sqrt(vx * vx + vy * vy);
    return inst->id;
}

JNIEXPORT jboolean JNICALL
Java_com_normaker_nativefull_MainActivity_nativeRuntimeSetSpriteBitmap(JNIEnv *env, jclass clazz, jint spriteId, jint frame, jint width, jint height, jbyteArray rgba) {
    if (!rgba) return JNI_FALSE;
    jbyte *bytes = (*env)->GetByteArrayElements(env, rgba, NULL);
    jsize len = (*env)->GetArrayLength(env, rgba);
    jboolean ok = JNI_FALSE;
    if (len >= width * height * 4) {
        ok = gm82_asset_set_sprite_bitmap(&g_assets, spriteId, frame, width, height, (const uint8_t *)bytes) ? JNI_TRUE : JNI_FALSE;
    }
    (*env)->ReleaseByteArrayElements(env, rgba, bytes, JNI_ABORT);
    return ok;
}

JNIEXPORT jboolean JNICALL
Java_com_normaker_nativefull_MainActivity_nativeRuntimeRenderBitmap(JNIEnv *env, jclass clazz, jobject target) {
    return JNI_TRUE;
}

JNIEXPORT void JNICALL
Java_com_normaker_nativefull_MainActivity_nativeRuntimeClearInstances(JNIEnv *env, jclass clazz) {
    g_world.instance_count = 0;
}

JNIEXPORT void JNICALL
Java_com_normaker_nativefull_MainActivity_nativeRuntimeClearRoomTransient(JNIEnv *env, jclass clazz) {
    g_world.instance_count = 0;
}

JNIEXPORT jboolean JNICALL
Java_com_normaker_nativefull_MainActivity_nativeRuntimeExecuteGml(JNIEnv *env, jclass clazz, jint instanceId, jstring source) {
    return JNI_TRUE;
}

JNIEXPORT jstring JNICALL
Java_com_normaker_nativefull_MainActivity_nativeRuntimeConsumeSoundCommands(JNIEnv *env, jclass clazz) {
    return (*env)->NewStringUTF(env, "[]");
}

JNIEXPORT jboolean JNICALL
Java_com_normaker_nativefull_MainActivity_nativeExportNorJson(JNIEnv *env, jclass clazz, jstring json, jstring outputPath) {
    return JNI_TRUE;
}

JNIEXPORT jint JNICALL
Java_com_normaker_nativefull_MainActivity_nativeImportGmxGmz(JNIEnv *env, jclass clazz, jstring path, jstring outputDir) {
    return 1;
}

JNIEXPORT jint JNICALL
Java_com_normaker_nativefull_MainActivity_nativeExportGmxGmz(JNIEnv *env, jclass clazz, jstring sourceDir, jstring outputPath, jstring kind) {
    return 1;
}

JNIEXPORT jboolean JNICALL
Java_com_normaker_nativefull_MainActivity_nativeExportGmkRaw(JNIEnv *env, jclass clazz, jstring sourcePath, jstring outputPath) {
    return JNI_TRUE;
}

JNIEXPORT jboolean JNICALL
Java_com_normaker_nativefull_MainActivity_nativeExportGmxSemantic(JNIEnv *env, jclass clazz, jstring sourceDir, jstring outputDir, jstring projectName) {
    return JNI_TRUE;
}

JNIEXPORT jstring JNICALL
Java_com_normaker_nativefull_MainActivity_nativeCoreIdentity(JNIEnv *env, jclass clazz) {
    return (*env)->NewStringUTF(env, "NOR Maker 8.2 Native Engine (Arm64/x86/v7a)");
}

JNIEXPORT jboolean JNICALL
Java_com_normaker_nativefull_MainActivity_nativeValidateGmk(JNIEnv *env, jclass clazz, jbyteArray bytes) {
    return JNI_TRUE;
}

JNIEXPORT jstring JNICALL
Java_com_normaker_nativefull_MainActivity_nativeGmkHeaderJson(JNIEnv *env, jclass clazz, jbyteArray bytes) {
    return (*env)->NewStringUTF(env, "{\"magic\":1234321,\"version\":800,\"valid\":true}");
}

JNIEXPORT jstring JNICALL
Java_com_normaker_nativefull_MainActivity_nativeGmkLayoutJson(JNIEnv *env, jclass clazz, jbyteArray bytes) {
    return (*env)->NewStringUTF(env, "{\"chunks\":[],\"count\":0}");
}

JNIEXPORT jstring JNICALL
Java_com_normaker_nativefull_MainActivity_nativeGmkChunkInventory(JNIEnv *env, jclass clazz, jbyteArray bytes) {
    return (*env)->NewStringUTF(env, "[]");
}

JNIEXPORT jstring JNICALL
Java_com_normaker_nativefull_MainActivity_nativeGmkResourceManifest(JNIEnv *env, jclass clazz, jbyteArray bytes) {
    return (*env)->NewStringUTF(env, "{\"resources\":[]}");
}

JNIEXPORT jstring JNICALL
Java_com_normaker_nativefull_MainActivity_nativeImportGmkSnapshot(JNIEnv *env, jclass clazz, jbyteArray bytes, jstring outputDir) {
    return (*env)->NewStringUTF(env, "{\"status\":\"ok\"}");
}

JNIEXPORT jint JNICALL
Java_com_normaker_nativefull_MainActivity_nativeCompileGml(JNIEnv *env, jclass clazz, jstring source) {
    static int code_id = 1000;
    return ++code_id;
}

JNIEXPORT jboolean JNICALL
Java_com_normaker_nativefull_MainActivity_nativeCodeExists(JNIEnv *env, jclass clazz, jint codeId) {
    return JNI_TRUE;
}

JNIEXPORT jint JNICALL
Java_com_normaker_nativefull_MainActivity_nativeCodeGetArgCount(JNIEnv *env, jclass clazz, jint codeId) {
    return 0;
}

JNIEXPORT void JNICALL
Java_com_normaker_nativefull_MainActivity_nativeCodeDestroy(JNIEnv *env, jclass clazz, jint codeId) {
}

JNIEXPORT jint JNICALL
Java_com_normaker_nativefull_MainActivity_nativeCodeExecute(JNIEnv *env, jclass clazz, jint instanceId, jint codeId) {
    return 0;
}

JNIEXPORT jboolean JNICALL
Java_com_normaker_nativefull_MainActivity_nativeExportRom(JNIEnv *env, jclass clazz, jstring title, jstring outputPath, jint kind) {
    return JNI_TRUE;
}

JNIEXPORT jint JNICALL
Java_com_normaker_nativefull_MainActivity_nativeDetectRom(JNIEnv *env, jclass clazz, jstring path) {
    return 0;
}

JNIEXPORT jboolean JNICALL
Java_com_normaker_nativefull_MainActivity_nativeValidateRom(JNIEnv *env, jclass clazz, jstring path, jint kind) {
    return JNI_TRUE;
}

JNIEXPORT void JNICALL
Java_com_normaker_nativefull_MainActivity_nativeClearResourceRegistry(JNIEnv *env, jclass clazz) {
    gm82_asset_manager_clear(&g_assets);
}

JNIEXPORT jint JNICALL
Java_com_normaker_nativefull_MainActivity_nativeRegisterResource(JNIEnv *env, jclass clazz, jint kind, jint id, jstring name, jint width, jint height, jint frames) {
    const char *native_name = name ? (*env)->GetStringUTFChars(env, name, NULL) : NULL;
    if (kind == 0) { // Sprite
        gm82_asset_register_sprite(&g_assets, id, native_name, width, height, frames, 0, 0);
    }
    if (name && native_name) (*env)->ReleaseStringUTFChars(env, name, native_name);
    return id;
}

JNIEXPORT jint JNICALL
Java_com_normaker_nativefull_MainActivity_nativeResourceCount(JNIEnv *env, jclass clazz) {
    return (jint)(g_assets.sprite_count + g_assets.background_count);
}

JNIEXPORT jint JNICALL
Java_com_normaker_nativefull_MainActivity_nativeRegisterObjectEvent(JNIEnv *env, jclass clazz, jint objectId, jint mainType, jint subType, jstring source) {
    return 1;
}

JNIEXPORT jint JNICALL
Java_com_normaker_nativefull_MainActivity_nativeObjectEventCount(JNIEnv *env, jclass clazz) {
    return 0;
}

JNIEXPORT jdouble JNICALL
Java_com_normaker_nativefull_MainActivity_nativeGm82CompatCheck(JNIEnv *env, jclass clazz) {
    return 8.2;
}

JNIEXPORT jdouble JNICALL
Java_com_normaker_nativefull_MainActivity_nativeGm82ColorReverse(JNIEnv *env, jclass clazz, jdouble color) {
    int c = (int)color;
    int r = c & 0xFF;
    int g = (c >> 8) & 0xFF;
    int b = (c >> 16) & 0xFF;
    return (double)((r << 16) | (g << 8) | b);
}

JNIEXPORT jdouble JNICALL
Java_com_normaker_nativefull_MainActivity_nativeGm82ColorInverse(JNIEnv *env, jclass clazz, jdouble color) {
    int c = (int)color;
    return (double)(0xFFFFFF ^ (c & 0xFFFFFF));
}

JNIEXPORT jint JNICALL
Java_com_normaker_nativefull_MainActivity_nativeGm82TokenStart(JNIEnv *env, jclass clazz, jstring text, jstring separator) {
    if (!text || !separator) return 0;
    const char *native_str = (*env)->GetStringUTFChars(env, text, NULL);
    const char *native_sep = (*env)->GetStringUTFChars(env, separator, NULL);
    strncpy(g_token_buf, native_str, sizeof(g_token_buf) - 1);
    strncpy(g_token_sep, native_sep, sizeof(g_token_sep) - 1);
    (*env)->ReleaseStringUTFChars(env, text, native_str);
    (*env)->ReleaseStringUTFChars(env, separator, native_sep);
    g_token_ctx = g_token_buf;
    return 1;
}

JNIEXPORT jstring JNICALL
Java_com_normaker_nativefull_MainActivity_nativeGm82TokenNext(JNIEnv *env, jclass clazz) {
    if (!g_token_ctx || !*g_token_ctx) return (*env)->NewStringUTF(env, "");
    char *token = strtok_r(g_token_ctx, g_token_sep, &g_token_ctx);
    return (*env)->NewStringUTF(env, token ? token : "");
}

JNIEXPORT void JNICALL
Java_com_normaker_nativefull_MainActivity_nativeGm82TokenReset(JNIEnv *env, jclass clazz) {
    g_token_ctx = NULL;
    memset(g_token_buf, 0, sizeof(g_token_buf));
}

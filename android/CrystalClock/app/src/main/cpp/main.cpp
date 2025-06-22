#include "raylib.h"
#include "../../../../../../src/clock.h"

#include "jni.h"
#include "egl_helper.h"

#include "android/native_window.h"
#include "android/asset_manager.h"
#include "android/native_window_jni.h"
#include "android/asset_manager_jni.h"
#include "android_native.h"

#include <utility>
#include <string>

const int TEXT_SIZE_DIP = 15;

int GetTextSize(int dip)
{
    struct android_app* app = GetAndroidApp();

    JavaVM* vm  = app->activity->vm;
    JNIEnv* env = nullptr;
    vm->AttachCurrentThread(&env, nullptr);

    jobject activity     = app->activity->clazz;
    jclass activityClass = env->GetObjectClass(activity);

    jmethodID getTextSizeMethod = env->GetMethodID(activityClass, "getTextSize",  "(I)I");
    jint size = env->CallIntMethod(activity, getTextSizeMethod, dip);

    env->DeleteLocalRef(activityClass);
    vm->DetachCurrentThread();

    return size;
}

std::pair<int, int> GetDisplayResolution()
{
    struct android_app* app = GetAndroidApp();

    JavaVM* vm  = app->activity->vm;
    JNIEnv* env = nullptr;
    vm->AttachCurrentThread(&env, nullptr);

    jobject activity     = app->activity->clazz;
    jclass activityClass = env->GetObjectClass(activity);

    jmethodID getDisplayWidthMethod  = env->GetMethodID(activityClass, "getDisplayWidth",  "()I");
    jmethodID getDisplayHeightMethod = env->GetMethodID(activityClass, "getDisplayHeight", "()I");

    jint width  = env->CallIntMethod(activity, getDisplayWidthMethod);
    jint height = env->CallIntMethod(activity, getDisplayHeightMethod);

    env->DeleteLocalRef(activityClass);
    vm->DetachCurrentThread();

    return std::pair<int, int> { width, height };
}

int main(int argc, char** argv)
{
    std::pair<int, int> res = GetDisplayResolution();
    int textSizePx = GetTextSize(TEXT_SIZE_DIP);

    SetTextSize(textSizePx);
    SetWindowResolution(res.first, res.second);
    SetFadeIn(false);
    SetShowTime(false);

    if (!Initialize())
        return 1;

    while (!WindowShouldClose())
        Loop();

    Uninitialize();
    return 0;
}

ANativeWindow* wallpaperWindow = NULL;
AAssetManager* aAssetManager   = NULL;
const char* filesDirPath       = NULL;

extern "C"
JNIEXPORT void JNICALL
Java_com_rrtry_crystalclock_RLWallpaperService_nativeInit(JNIEnv *env, jobject thiz,
                                                          jobject surface, jobject asset_manager,
                                                          jstring internal_data_path) {

    wallpaperWindow = ANativeWindow_fromSurface(env, surface);
    aAssetManager   = AAssetManager_fromJava(env, asset_manager);

    const char* chars = env->GetStringUTFChars(internal_data_path, 0);
    filesDirPath      = strdup(chars);

    env->ReleaseStringUTFChars(internal_data_path, chars);

    SetWindowResolution(ANativeWindow_getWidth(wallpaperWindow), ANativeWindow_getHeight(wallpaperWindow));
    SetFadeIn(false);
    SetShowTime(false);

    LoadEGLContext();
    Initialize();
}

extern "C"
JNIEXPORT void JNICALL
Java_com_rrtry_crystalclock_RLWallpaperService_nativeRender(JNIEnv *env, jobject thiz) {
    Loop();
}

extern "C"
JNIEXPORT void JNICALL
Java_com_rrtry_crystalclock_RLWallpaperService_nativeShutdown(JNIEnv *env, jobject thiz) {
    Uninitialize();
    ANativeWindow_release(wallpaperWindow);
    wallpaperWindow = NULL;
}
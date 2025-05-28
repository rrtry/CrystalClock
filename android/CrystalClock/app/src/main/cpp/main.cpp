#include "raylib.h"
#include "jni.h"
#include "android_native.h"
#include "../../../../../../src/clock.h"

#include <utility>

std::pair<int, int> GetDisplayResolution() {

    struct android_app *app = GetAndroidApp();

    JavaVM* vm  = app->activity->vm;
    JNIEnv* env = nullptr;
    vm->AttachCurrentThread(&env, nullptr);

    jobject activity = app->activity->clazz;
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
    SetWindowResolution(res.first, res.second);

    InitWindow();
    SetTimeLocale();
    InitCamera();

    struct android_app* app = GetAndroidApp();

    SetResourcesPath(app->activity->internalDataPath);
    LoadResources();
    SetRenderOptions();

    while (!WindowShouldClose())
        Loop();

    UnloadResources();
    CloseWindow();

    return 0;
}
﻿#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"
#include "timeinfo.h"
#include "config.h"

#include <iostream>
#include <clocale>
#include <vector>

#if defined(PLATFORM_WEB) || defined(PLATFORM_ANDROID)
    #define GLSL_VERSION 100
#else
    #define GLSL_VERSION 330
#endif

using namespace std;

const Vector3 PRISM_COLORS[] = {
    { 0.04f, 0.23f, 0.46f },
    { 0.17f, 0.03f, 0.45f },
    { 0.03f, 0.39f, 0.45f }
};

const int ORBS           = 7;
const int TRAIL_SEGMENTS = 120;

const float TRAIL_WIDTH       = 2.0f;
const float ORB_SCALE         = 2.5f;
const float MAX_SPHERE_RADIUS = 6.0f;
const float MIN_SPHERE_RADIUS = MAX_SPHERE_RADIUS / 2;

const float SPHERE_SCALE_TIME = 1.5f;
const float PRISM_SCALE_TIME  = 1.5f;
const float FADE_TIME         = 2.0f;
const float START_FADE_TIME   = 4.0f;
const float TRAIL_FADE_TIME   = 2500.f;

const float FIXED_FOV  = 70.f;
const float X_SPEED    = PI / 2;
const float Z_SPEED    = -PI;
const float ANGLE_STEP = 360.f / 60.f;
const float ANGLES[]   = { PI / 2, PI + PI / 6, 0.f };

const double CAMERA_NEAR_PLANE = 0.1;
const double CAMERA_FAR_PLANE  = 100.0;

//------------------------------------------------------------------------------------
// Window size
//------------------------------------------------------------------------------------
int screenWidth  = 0;
int screenHeight = 0;
int windowFlags  = 0;

//------------------------------------------------------------------------------------
// Camera
//------------------------------------------------------------------------------------
Camera camera;

//------------------------------------------------------------------------------------
// Config / Locale
//------------------------------------------------------------------------------------
Config cfg = { 0 };
const char* timeLocale;
int textSize = 30;

//------------------------------------------------------------------------------------
// Music stream
//------------------------------------------------------------------------------------
Music ambience;

//------------------------------------------------------------------------------------
// Framebuffers
//------------------------------------------------------------------------------------
RenderTexture tunnelLayer;
RenderTexture orbsLayer;
RenderTexture clockLayer;

//------------------------------------------------------------------------------------
// Models
//------------------------------------------------------------------------------------
Model prism;
Model tube;

//------------------------------------------------------------------------------------
// Textures
//------------------------------------------------------------------------------------
Texture2D normalTexture;
Texture2D noiseTexture;
Texture2D orbTexture;

//------------------------------------------------------------------------------------
// Shaders
//------------------------------------------------------------------------------------
Shader crystalShader;
Shader tunnelShader;
Shader orbShader;
Shader fxaaShader;

//------------------------------------------------------------------------------------
// Input variables
//------------------------------------------------------------------------------------

// Tunnel's model/normal marticies
Matrix TM = MatrixMultiply(MatrixRotateX(PI + PI / 2), MatrixTranslate(0.f, 0.f, 30.f));
Matrix TN = MatrixInvert(MatrixTranspose(TM));

// Time structs
Time currentTime;
ElapsedSeconds elapsedSeconds;

Vector3 clockPosition = { 0.0f, MAX_SPHERE_RADIUS + 0.5f, 0.0f };
Vector3 prismColor;

float deltaTime;
float elapsedTime;     // Time since start
float secondsInMinute; // Orbs position
float secondsInHour;   // 'Time left' indicator for crystal rods

float clockMinuteRotation; // (Y-rotation) Clock makes full rotation every minute
float clockHourRotation;   // (Z-Rotation) Rotate along the rod that represents current hour

float sphereRadius; // Distance from the center for orbs
float prismScale;   // Y-scale for 'time left' indicator

float sphereRadiusAnim = 0.f;
float prismScaleAnim   = 0.f;
float fadeAnim         = 0.f;

bool newHour;
bool fading;

bool showClock = true;
bool fadeIn    = true;
bool showTime  = true;
bool playSound = true;

Color clockLayerTint = WHITE;
Color orbLayerTint   = WHITE;

//------------------------------------------------------------------------------------
// Gesture contols
//------------------------------------------------------------------------------------
int currentGesture = GESTURE_NONE;
int lastGesture    = GESTURE_NONE;

//------------------------------------------------------------------------------------
// Math functions
//------------------------------------------------------------------------------------
float GetOrbRotationAngle(float time, int i)
{
    return time * i * ANGLE_STEP * DEG2RAD;
}

float GetClockRotationAngle(float hour)
{
    return hour < 12 ? hour * -30.0f : (hour - 12) * -30.0f;
}

float GetCurrentHourRotationAngle(const ElapsedSeconds& s)
{
    float elapsedHours = (float)fmod(s.day / 3600, 24);
    return (90.f + GetClockRotationAngle(elapsedHours)) * DEG2RAD;
}

float LerpXRotationAngle(const ElapsedSeconds& s, const Time& t)
{
    return Lerp(ANGLES[t.minute % 3], ANGLES[(t.minute + 1) % 3], Normalize(s.minute, 0.f, 60.f));
}

Matrix GetRotationMatrix(const ElapsedSeconds& s, const Time& t, float hourAngle)
{
    float ax = X_SPEED * s.minute + LerpXRotationAngle(s, t);
    float az = Z_SPEED * s.minute;

    Matrix rx = MatrixRotateX(ax);
    Matrix rz = MatrixRotateZ(az);

    return MatrixMultiply(MatrixMultiply(rz, rx), MatrixRotateZ(hourAngle));
}

Vector3 GetOrbPosition(float time, float radius, int orbIndex, const Matrix& rotation)
{
    float angle = GetOrbRotationAngle(time, orbIndex);
    return Vector3Transform(
            { radius * cosf(angle), radius * sinf(angle), 0.f },
            rotation
    );
}

Vector3 GetOrbPosition(const TimePoint& prevTimePoint, float radius, float hourAngle, int orbIndex)
{
    Time prevTime;
    ElapsedSeconds prevSeconds;

    GetTimeInfo(&prevTime, prevTimePoint);
    GetElapsedSeconds(&prevSeconds, prevTime);

    Matrix rotation = GetRotationMatrix(prevSeconds, prevTime, hourAngle);
    return GetOrbPosition(prevSeconds.minute, radius, orbIndex, rotation);
}

//------------------------------------------------------------------------------------
// Drawing functions
//------------------------------------------------------------------------------------
float GetSegmentAlpha(const TimePoint& prevTimePoint)
{
    auto now    = currentTime.timePoint;
    auto diff   = now - prevTimePoint;
    auto millis = chrono::duration_cast<chrono::milliseconds>(diff).count();
    return Lerp(1.0f, 0.0f, Normalize(millis, 0.f, TRAIL_FADE_TIME));
}

void DrawTrailSegment(TimePoint& prevTimePoint, Vector3& lastPos, float radius, float hourAngle, int orbIndex)
{
    Vector3 prevPosition = GetOrbPosition(prevTimePoint, radius, hourAngle, orbIndex);
    prevTimePoint -= chrono::milliseconds((int)(deltaTime * 1000.f));

    DrawLine3D(lastPos, prevPosition, Fade(WHITE, GetSegmentAlpha(prevTimePoint)));
    lastPos = prevPosition;
}

void DrawTrail(TimePoint prevTimePoint, float radius, float hourAngle, int orbIndex)
{
    Vector3 lastPosition = GetOrbPosition(prevTimePoint, radius, hourAngle, orbIndex);
    for (int j = 0; j < TRAIL_SEGMENTS; j++)
    {
        DrawTrailSegment(prevTimePoint, lastPosition, radius, hourAngle, orbIndex);
    }
}

void DrawOrbs(float radius)
{
    float hourAngle = GetCurrentHourRotationAngle(elapsedSeconds);
    Matrix rotation = GetRotationMatrix(elapsedSeconds, currentTime, hourAngle);
    Vector3 orbPosition;

    for (int i = 0; i < ORBS; i++)
    {
        orbPosition = GetOrbPosition(elapsedSeconds.minute, radius, i, rotation);
        SetShaderValue(
                prism.materials[0].shader,
                GetShaderLocation(prism.materials[0].shader, TextFormat("pointLights[%d].position", i)),
                &orbPosition,
                RL_SHADER_UNIFORM_VEC3
        );

        DrawBillboard(camera, orbTexture, orbPosition, ORB_SCALE, WHITE);
        DrawBillboard(camera, orbTexture, orbPosition, ORB_SCALE * 0.5, WHITE);
        DrawTrail(currentTime.timePoint, radius, hourAngle, i);
    }
}

void DrawClock(float secOfMinRotation, float hourOfDayRotation, float hourPrismScale)
{
    float angle = 0.0f;
    for (int i = 0; i < 12; i++)
    {
        Matrix M, N, R;
        R = MatrixRotateY(secOfMinRotation * 4.f * DEG2RAD);
        M = MatrixIdentity();
        M = MatrixMultiply(M, MatrixRotateZ(angle * DEG2RAD));
        M = MatrixMultiply(M, MatrixRotateY(-secOfMinRotation * DEG2RAD));
        M = MatrixMultiply(M, MatrixRotateZ(hourOfDayRotation * DEG2RAD));
        M = MatrixMultiply(R, M);
        N = MatrixTranspose(MatrixInvert(M));

        rlPushMatrix();

        rlMultMatrixf(MatrixToFloat(M));
        SetShaderValueMatrix(prism.materials[0].shader, GetShaderLocation(prism.materials[0].shader, "model"),   M);
        SetShaderValueMatrix(prism.materials[0].shader, GetShaderLocation(prism.materials[0].shader, "mNormal"), N);

        if (i == 0)
        {
            rlDisableDepthMask();
            DrawModelEx(
                    prism,
                    clockPosition,
                    { 0.f, 0.f, 0.f },
                    0.f,
                    { 1.f, hourPrismScale, 1.f },
                    WHITE
            );
            rlEnableDepthMask();
        }
        DrawModel(prism, clockPosition, 1.f, WHITE);

        rlPopMatrix();
        angle -= 30.f;
    }
}

void DrawDateTime()
{
    string dateStr = FormatDate(currentTime, timeLocale);
    string timeStr = FormatTime(currentTime, timeLocale);

    int timeSize = MeasureText(timeStr.c_str(), textSize);
    DrawText(dateStr.c_str(), 10, 10, textSize, WHITE);
    DrawText(timeStr.c_str(), screenWidth - timeSize - 10, 10, textSize, WHITE);
}

//------------------------------------------------------------------------------------
// Interpolation functions
//------------------------------------------------------------------------------------
Vector3 LerpPrismColor(float t)
{
    float mod = fmod(t, 10);
    int color = ((int)t - (int)mod) % 3;
    return Vector3Lerp(
            PRISM_COLORS[color],
            PRISM_COLORS[(color + 1) % 3],
            mod / 10
    );
}

float LerpClockRotation(float t)
{
    return Lerp(0.f, 360.f, Normalize(t, 0.f, 60.f));
}

float LerpPrismScale(float t)
{
    return 1.f - t / 3600.f;
}

float LerpSphereRadius(float t)
{
    return Lerp(MIN_SPHERE_RADIUS, MAX_SPHERE_RADIUS, Normalize(t, 0.f, 3600.f));
}

float InvLerpSphereRadius(float t)
{
    return Lerp(MAX_SPHERE_RADIUS, MIN_SPHERE_RADIUS, Normalize(t, 0.f, SPHERE_SCALE_TIME));
}

float InvLerpPrismScale(float t)
{
    return Lerp(0.f, 1.f, Normalize(t, 0.f, PRISM_SCALE_TIME));
}

//------------------------------------------------------------------------------------
// Game loop / Initialization functions
//------------------------------------------------------------------------------------
int GetTargetDisplay()
{
    return cfg.display;
}

void SetShowTime(bool show)
{
    showTime = show;
}

void SetPlaySound(bool play)
{
    playSound = play;
}

void SetFadeIn(bool fade)
{
    clockLayerTint = fade ? BLACK : WHITE;
    orbLayerTint   = fade ? BLACK : WHITE;
    fadeIn = fade;
}

void SetTextSize(int px)
{
    textSize = px;
}

void SetWindowResolution(int width, int height)
{
    screenWidth  = width;
    screenHeight = height;
}

void SetTimeLocale()
{
    setlocale(LC_ALL, "");
    timeLocale = setlocale(LC_TIME, nullptr);
}

bool ParseConfig(int argc, char** argv, bool prefsOnly)
{
    string err = "";
    if (!ParseCMD(cfg, argc, argv, err, prefsOnly))
    {
        if (!ParseINI(cfg, "resources/config.ini", prefsOnly))
        {
            cerr << "Could not read config" << endl;
            return false;
        }
    }

    if (!prefsOnly)
    {
        screenWidth  = cfg.screenWidth;
        screenHeight = cfg.screenHeight;
        windowFlags  = cfg.flags;
    }

    playSound = (cfg.preferenceFlags & FLAG_NO_SOUND)   == 0;
    fadeIn    = (cfg.preferenceFlags & FLAG_NO_FADE_IN) == 0;

    return true;
}

float GetVerticalFOV()
{
    if (screenHeight > screenWidth)
    {
        float aspect = (float)screenWidth / (float)screenHeight;
        return 2.0f * atanf(tanf(FIXED_FOV * DEG2RAD * 0.5f) / aspect) * RAD2DEG;
    }
    return FIXED_FOV;
}

void InitCamera()
{
    camera = { 0 };
    Vector3 position = { 0.0f, 0.0f, 30.f };
    Vector3 target   = { 0.0f, 0.0f, -1.f  };
    Vector3 up       = { 0.0f, 1.0f, 0.0f  };

    camera.position = position;
    camera.target   = target;
    camera.up       = up;

    camera.fovy = GetVerticalFOV();
    camera.projection = CAMERA_PERSPECTIVE;
    SetTargetFPS(60);
}

void InitWindow()
{
    SetConfigFlags(windowFlags | FLAG_WINDOW_RESIZABLE);
    InitWindow(screenWidth, screenHeight, "CrystalClock");

    if (screenWidth == 0 || screenHeight == 0)
    {
        screenWidth  = GetScreenWidth();
        screenHeight = GetScreenHeight(); 
    }
    
#if defined(PLATFORM_DESKTOP) && !defined(WALLPAPER)

    if (cfg.display <= 0)
        return;
    //------------------------------------------------------------------------------------
    // Retrieve display count
    //------------------------------------------------------------------------------------
    int display      = cfg.display;
    int displayCount = GetMonitorCount();

    //------------------------------------------------------------------------------------
    // Attempting to move window to specified display
    //------------------------------------------------------------------------------------
    if (display < displayCount && display > -1)
        SetWindowMonitor(display);
#endif
}

void LoadAudio()
{
    if (playSound)
    {
        InitAudioDevice();
        ambience = LoadMusicStream("resources/sound/waves.ogg");
        ambience.looping = true;
        PlayMusicStream(ambience);
    }
}

void SetShaderResolution()
{
    Vector2 res = { (float)screenWidth, (float)screenHeight };
    SetShaderValue(fxaaShader, GetShaderLocation(fxaaShader, "resolution"), &res, SHADER_UNIFORM_VEC2);
}

void LoadResources()
{
    //------------------------------------------------------------------------------------
    // Load audio
    //------------------------------------------------------------------------------------
    LoadAudio();

    //------------------------------------------------------------------------------------
    // Textures/models
    //------------------------------------------------------------------------------------
    tunnelLayer = LoadRenderTexture(screenWidth, screenHeight);
    orbsLayer   = LoadRenderTexture(screenWidth, screenHeight);
    clockLayer  = LoadRenderTexture(screenWidth, screenHeight);

    prism = LoadModel("resources/prism.obj");
    tube  = LoadModelFromMesh(GenMeshCylinder(20.f, 100, 30));

    normalTexture = LoadTexture("resources/textures/normal.jpg");
    noiseTexture  = LoadTexture("resources/textures/noiseTexture.png");
    orbTexture    = LoadTexture("resources/textures/halo.png");

    int normalMap = MATERIAL_MAP_NORMAL;
    GenMeshTangents(&(prism.meshes[0]));

    //------------------------------------------------------------------------------------
    // Shaders/materials
    //------------------------------------------------------------------------------------
    string glslDirectory = "resources/shaders/glsl" + to_string(GLSL_VERSION);
    crystalShader = LoadShader((glslDirectory + "/crystal.vs").c_str(), (glslDirectory + "/crystal.fs").c_str());
    tunnelShader  = LoadShader((glslDirectory + "/tunnel.vs").c_str(),  (glslDirectory + "/tunnel.fs").c_str());
    orbShader     = LoadShader(0, (glslDirectory + "/orb.fs").c_str());
    fxaaShader    = LoadShader(0, "resources/shaders/fxaa.fs");

    //------------------------------------------------------------------------------------
    // Set resolution for antialiasing shader
    //------------------------------------------------------------------------------------
    SetShaderResolution();

    //------------------------------------------------------------------------------------
    // Crystal rod
    //------------------------------------------------------------------------------------
    Vector3 crystalAmbient   = { 0.04f, 0.23f, 0.46f };
    Vector3 crystalDiffuse   = { 0.04f, 0.23f, 0.46f };
    Vector3 crystalSpecular  = { 1.0f,  1.0f,  1.0f  };
    float   crystalShininess = 0.4f;

    Vector3 orbLightAmbient   = { 0.5f, 0.5f, 0.5f };
    Vector3 orbLightDiffuse   = { 0.8f, 0.8f, 0.8f };
    Vector3 orbLightSpecular  = { 1.0f, 1.0f, 1.0f };

    Vector3 dirLightAmbient  = { 0.5f, 0.5f,  0.5f  };
    Vector3 dirLightDiffuse  = { 0.8f, 0.8f,  0.8f  };
    Vector3 dirLightSpecular = { 0.5f, 0.5f,  0.5f  };
    Vector3 lightDirection   = { 0.0f, 0.0f,  -1.0f };

    SetShaderValue(crystalShader, GetShaderLocation(crystalShader, "material.ambient"),   &crystalAmbient,   RL_SHADER_UNIFORM_VEC3);
    SetShaderValue(crystalShader, GetShaderLocation(crystalShader, "material.diffuse"),   &crystalDiffuse,   RL_SHADER_UNIFORM_VEC3);
    SetShaderValue(crystalShader, GetShaderLocation(crystalShader, "material.specular"),  &crystalSpecular,  RL_SHADER_UNIFORM_VEC3);
    SetShaderValue(crystalShader, GetShaderLocation(crystalShader, "material.shininess"), &crystalShininess, RL_SHADER_UNIFORM_FLOAT);

    SetShaderValue(crystalShader, GetShaderLocation(crystalShader, "dirLight.direction"), &lightDirection,    RL_SHADER_UNIFORM_VEC3);
    SetShaderValue(crystalShader, GetShaderLocation(crystalShader, "dirLight.ambient"),   &dirLightAmbient,   RL_SHADER_UNIFORM_VEC3);
    SetShaderValue(crystalShader, GetShaderLocation(crystalShader, "dirLight.diffuse"),   &dirLightDiffuse,   RL_SHADER_UNIFORM_VEC3);
    SetShaderValue(crystalShader, GetShaderLocation(crystalShader, "dirLight.specular"),  &dirLightSpecular,  RL_SHADER_UNIFORM_VEC3);

    // 13	1.0	0.35	0.44
    const float ORB_LIGHT_KC = 1.0f;
    const float ORB_LIGHT_KL = 0.7f;
    const float ORB_LIGHT_KQ = 1.8f;

    for (int i = 0; i < ORBS; i++)
    {
        SetShaderValue(crystalShader, GetShaderLocation(crystalShader, TextFormat("pointLights[%d].ambient",   i)), &orbLightAmbient,  RL_SHADER_UNIFORM_VEC3);
        SetShaderValue(crystalShader, GetShaderLocation(crystalShader, TextFormat("pointLights[%d].diffuse",   i)), &orbLightDiffuse,  RL_SHADER_UNIFORM_VEC3);
        SetShaderValue(crystalShader, GetShaderLocation(crystalShader, TextFormat("pointLights[%d].specular",  i)), &orbLightSpecular, RL_SHADER_UNIFORM_VEC3);

        SetShaderValue(crystalShader, GetShaderLocation(crystalShader, TextFormat("pointLights[%d].constant",  i)), &ORB_LIGHT_KC, RL_SHADER_UNIFORM_FLOAT);
        SetShaderValue(crystalShader, GetShaderLocation(crystalShader, TextFormat("pointLights[%d].linear",    i)), &ORB_LIGHT_KL, RL_SHADER_UNIFORM_FLOAT);
        SetShaderValue(crystalShader, GetShaderLocation(crystalShader, TextFormat("pointLights[%d].quadratic", i)), &ORB_LIGHT_KQ, RL_SHADER_UNIFORM_FLOAT);
    }

    prism.materials[0].shader = crystalShader;
    SetShaderValue(prism.materials[0].shader, GetShaderLocation(prism.materials[0].shader, "normalMap"), &normalMap, SHADER_UNIFORM_INT);
    prism.materials[0].maps[MATERIAL_MAP_NORMAL].texture = normalTexture;

    //------------------------------------------------------------------------------------
    // Tunnel
    //------------------------------------------------------------------------------------
    const float TUNNEL_LIGHT_KC = 1.f;
    const float TUNNEL_LIGHT_KL = 0.007f;
    const float TUNNEL_LIGHT_KQ = 0.0002f;

    Vector3 tunnelAmbient = { 0.1f, 0.1f, 0.1f };
    Vector3 tunnelDiffuse = { 1.0f, 1.0f, 1.0f };

    SetShaderValue(tunnelShader, GetShaderLocation(tunnelShader, "tunlight.ambient"), &tunnelAmbient, SHADER_UNIFORM_VEC3);
    SetShaderValue(tunnelShader, GetShaderLocation(tunnelShader, "tunlight.diffuse"), &tunnelDiffuse, SHADER_UNIFORM_VEC3);

    SetShaderValue(tunnelShader, GetShaderLocation(tunnelShader, "tunlight.constant"),  &TUNNEL_LIGHT_KC, SHADER_UNIFORM_FLOAT);
    SetShaderValue(tunnelShader, GetShaderLocation(tunnelShader, "tunlight.linear"),    &TUNNEL_LIGHT_KL, SHADER_UNIFORM_FLOAT);
    SetShaderValue(tunnelShader, GetShaderLocation(tunnelShader, "tunlight.quadratic"), &TUNNEL_LIGHT_KQ, SHADER_UNIFORM_FLOAT);

    tube.materials[0].shader = tunnelShader;
    tube.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = noiseTexture;
}

void UnloadResources()
{
    if (playSound)
    {
        StopMusicStream(ambience);
        UnloadMusicStream(ambience);
        CloseAudioDevice();
    }

    UnloadTexture(orbTexture);
    UnloadTexture(noiseTexture);
    UnloadTexture(normalTexture);

    UnloadShader(crystalShader);
    UnloadShader(orbShader);
    UnloadShader(tunnelShader);
    UnloadShader(fxaaShader);

    UnloadModel(prism);
    UnloadModel(tube);

    UnloadRenderTexture(tunnelLayer);
    UnloadRenderTexture(orbsLayer);
    UnloadRenderTexture(clockLayer);
}

void SetRenderOptions()
{
    rlEnableColorBlend();
    rlEnableSmoothLines();
    rlDisableBackfaceCulling();

    rlSetClipPlanes(CAMERA_NEAR_PLANE, CAMERA_FAR_PLANE);
    rlSetLineWidth(TRAIL_WIDTH);
}

void ResizeWindow()
{
    if (IsWindowHidden() || IsWindowMinimized())
    {
        return;
    }
    if (IsWindowResized() && !IsWindowFullscreen())
    {
        screenWidth  = GetScreenWidth();
        screenHeight = GetScreenHeight();

        UnloadRenderTexture(tunnelLayer);
        UnloadRenderTexture(orbsLayer);
        UnloadRenderTexture(clockLayer);

        tunnelLayer = LoadRenderTexture(screenWidth, screenHeight);
        orbsLayer   = LoadRenderTexture(screenWidth, screenHeight);
        clockLayer  = LoadRenderTexture(screenWidth, screenHeight);

        SetWindowSize(screenWidth, screenHeight);
        SetShaderResolution();
        camera.fovy = GetVerticalFOV();
    }
}

void HandleControls()
{
    fading = IsKeyPressed(KEY_J) || fadeAnim > 0.f;
    if (IsKeyPressed(KEY_K))
        showTime = !showTime;
    
#if defined(PLATFORM_ANDROID) || defined(PLATFORM_WEB)

    lastGesture    = currentGesture;
    currentGesture = GetGestureDetected();

    if (currentGesture != GESTURE_NONE)
    {
        if (currentGesture != lastGesture)
        {
            switch (currentGesture)
            {
            case GESTURE_SWIPE_DOWN:
                fading = true || fadeAnim > 0.f;
                break;
            case GESTURE_SWIPE_UP:
                showTime = !showTime;
                break;
            default:
                break;
            }   
        }
    }
#endif
}

void Update()
{
    if (playSound)
        UpdateMusicStream(ambience);
    
    GetTimeInfo(&currentTime);
    GetElapsedSeconds(&elapsedSeconds, currentTime);

    deltaTime       = GetFrameTime();
    elapsedTime     = (float)GetTime();
    secondsInMinute = elapsedSeconds.minute;
    secondsInHour   = elapsedSeconds.hour;

    prismColor          = LerpPrismColor(elapsedSeconds.minute);
    clockMinuteRotation = LerpClockRotation(secondsInMinute);
    clockHourRotation   = GetClockRotationAngle(currentTime.hour);
    newHour             = (int)roundf(secondsInHour) == 0 || sphereRadiusAnim > 0.f || prismScaleAnim > 0.f;

    //------------------------------------------------------------------------------------
    // Controls
    //------------------------------------------------------------------------------------
    HandleControls();

    //------------------------------------------------------------------------------------
    // Animations
    //------------------------------------------------------------------------------------
    if (elapsedTime < START_FADE_TIME && fadeIn)
    {
        Vector3 tint = Vector3Lerp(
                Vector3({ 0, 0, 0 }),
                Vector3({ 255, 255, 255 }),
                elapsedTime / START_FADE_TIME
        );

        Color color    = { (unsigned char)tint.x, (unsigned char)tint.y, (unsigned char)tint.z, 255 };
        clockLayerTint = color;
        orbLayerTint   = color;
    }
    else if (fading)
    {
        Vector3 color = Vector3Lerp(
                showClock ? Vector3({ 255, 255, 255 }) : Vector3({ 0, 0, 0 }),
                showClock ? Vector3({ 0, 0, 0 }) : Vector3({ 255, 255, 255 }),
                fadeAnim / FADE_TIME
        );

        clockLayerTint = { (unsigned char)color.x, (unsigned char)color.y, (unsigned char)color.z, 255 };
        fadeAnim += deltaTime;

        if (fadeAnim > FADE_TIME)
        {
            fadeAnim  = 0.f;
            showClock = !showClock;
        }
    }

    if (newHour)
    {
        sphereRadius = InvLerpSphereRadius(sphereRadiusAnim);
        sphereRadiusAnim += deltaTime;

        prismScale = InvLerpPrismScale(prismScaleAnim);
        prismScaleAnim += deltaTime;

        if (sphereRadiusAnim > SPHERE_SCALE_TIME)
            sphereRadiusAnim = 0.f;

        if (prismScaleAnim > PRISM_SCALE_TIME)
            prismScaleAnim = 0.f;
    }
    else
    {
        sphereRadius = LerpSphereRadius(secondsInHour);
        prismScale   = LerpPrismScale(secondsInHour);
    }
}

void Render()
{
#if !defined(WALLPAPER)
    ResizeWindow();
#endif
    //------------------------------------------------------------------------------------
    // Set shader uniforms
    //------------------------------------------------------------------------------------
    if (showClock || fading)
    {
        float wrappedTime = fmod(elapsedTime * 0.004f, 1.0f);
        SetShaderValue(tube.materials[0].shader, GetShaderLocation(tube.materials[0].shader, "time"), &wrappedTime, SHADER_UNIFORM_FLOAT);

        SetShaderValueMatrix(tube.materials[0].shader, GetShaderLocation(tube.materials[0].shader, "model"),   TM);
        SetShaderValueMatrix(tube.materials[0].shader, GetShaderLocation(tube.materials[0].shader, "mNormal"), TN);

        SetShaderValue(prism.materials[0].shader, GetShaderLocation(prism.materials[0].shader, "material.diffuse"), &prismColor,        RL_SHADER_UNIFORM_VEC3);
        SetShaderValue(prism.materials[0].shader, GetShaderLocation(prism.materials[0].shader, "material.ambient"), &prismColor,        RL_SHADER_UNIFORM_VEC3);
        SetShaderValue(prism.materials[0].shader, GetShaderLocation(prism.materials[0].shader, "viewPos"),          &(camera.position), RL_SHADER_UNIFORM_VEC3);

        SetShaderValue(tube.materials[0].shader, GetShaderLocation(tube.materials[0].shader, "viewPos"),           &(camera.position),  RL_SHADER_UNIFORM_VEC3);
        SetShaderValue(tube.materials[0].shader, GetShaderLocation(tube.materials[0].shader, "tunlight.position"), &(camera.position),  RL_SHADER_UNIFORM_VEC3);
    }
    //------------------------------------------------------------------------------------
    // Render
    //------------------------------------------------------------------------------------
    rlSetBlendMode(RL_BLEND_ALPHA);
    //------------------------------------------------------------------------------------
    // Tunnel layer
    //------------------------------------------------------------------------------------
    if (showClock || fading)
    {
        rlSetCullFace(RL_CULL_FACE_FRONT);
        rlEnableBackfaceCulling();

        BeginTextureMode(tunnelLayer);
            BeginMode3D(camera);
                ClearBackground(BLACK);
                rlPushMatrix();
                    rlMultMatrixf(MatrixToFloat(TM));
                    DrawModel(tube, { 0.f, 0.f, 0.f }, 1.0f, Fade(WHITE, 0.0f));
                rlPopMatrix();
            EndMode3D();
        EndTextureMode();

        rlDisableBackfaceCulling();
        rlSetCullFace(RL_CULL_FACE_BACK);
    }

    //------------------------------------------------------------------------------------
    // Orbs layer
    //------------------------------------------------------------------------------------
    rlSetBlendMode(RL_BLEND_ADDITIVE);
    BeginTextureMode(orbsLayer);
        BeginMode3D(camera);

            ClearBackground(Fade(BLACK, 0.0));
            rlDisableDepthMask();

            BeginShaderMode(orbShader);
            DrawOrbs(sphereRadius);
            EndShaderMode();

            rlEnableDepthMask();
        EndMode3D();
    EndTextureMode();

    //------------------------------------------------------------------------------------
    // Clock layer
    //------------------------------------------------------------------------------------
    if (showClock || fading)
    {
        BeginTextureMode(clockLayer);
            BeginMode3D(camera);
                ClearBackground(Fade(BLACK, 0.0));
                DrawClock(clockMinuteRotation, clockHourRotation, prismScale);
            EndMode3D();
        EndTextureMode();
    }

    //------------------------------------------------------------------------------------
    // Blend three layers
    //------------------------------------------------------------------------------------
    BeginDrawing();
    ClearBackground(BLACK);
    if (showClock || fading)
    {
        rlSetBlendMode(RL_BLEND_ALPHA);
        DrawTextureRec(tunnelLayer.texture, { 0, 0, (float)screenWidth, (float) -screenHeight}, {0, 0}, clockLayerTint);

        rlSetBlendMode(RL_BLEND_ADDITIVE);
        if (showClock && !fading && (elapsedTime > START_FADE_TIME || !fadeIn))
        {
            BeginShaderMode(fxaaShader);
                DrawTextureRec(clockLayer.texture, { 0, 0, (float)screenWidth, (float) -screenHeight}, {0, 0}, clockLayerTint);
            EndShaderMode();
        }
        else
        {
            DrawTextureRec(clockLayer.texture, { 0, 0, (float)screenWidth, (float) -screenHeight}, {0, 0}, clockLayerTint);
        }

        if (showTime)
            DrawDateTime();
    }
    DrawTextureRec(orbsLayer.texture,  { 0, 0, (float)screenWidth, (float) -screenHeight}, {0, 0}, orbLayerTint);
    EndDrawing();
}

bool Initialize()
{
    if (screenWidth <= 0 || screenHeight <= 0)
    {
        return false;
    }
    //------------------------------------------------------------------------------------
    // Set time locale
    //------------------------------------------------------------------------------------
    SetTimeLocale();

    //------------------------------------------------------------------------------------
    // Window initialization
    //------------------------------------------------------------------------------------
    InitWindow();

    //------------------------------------------------------------------------------------
    // Camera initialization
    //------------------------------------------------------------------------------------
    InitCamera();

    //------------------------------------------------------------------------------------
    // Loading main models and textures
    //------------------------------------------------------------------------------------
    LoadResources();

    //------------------------------------------------------------------------------------
    // Setting render options
    //------------------------------------------------------------------------------------
    SetRenderOptions();
    return true;
}

bool Initialize(int argc, char** argv)
{
    //------------------------------------------------------------------------------------
    // Parse command line parameters
    //------------------------------------------------------------------------------------
    if (!ParseConfig(argc, argv, false))
        return false;
    
    return Initialize();
}

void Uninitialize()
{
    UnloadResources();
    CloseWindow();
}

void Loop()
{
    Update();
    Render();
}
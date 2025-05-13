#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"
#include "timeinfo.h"
#include "config.h"

#include <iostream>
#include <clocale>

#if defined(PLATFORM_WEB)
    #define GLSL_VERSION 100
#else
    #define GLSL_VERSION 330
#endif

using namespace std;

int screenWidth  = 1920;
int screenHeight = 1080;

const Vector3 PRISM_COLORS[] = {
    { 0.04, 0.23, 0.46 },
    { 0.17, 0.03, 0.45 },
    { 0.03, 0.39, 0.45 }
};

const Color BACKGROUND = { 82,  32,  129, 255 };
const Color ORBCOLOR   = { 163, 239, 255, 255 };

const int NUM_PRISM_COLORS = sizeof(PRISM_COLORS) / sizeof(Vector3);

const int ORBS         = 7;
const int TRAIL_LENGTH = 25;

const float MIN_SPHERE_RADIUS = 2.5f;
const float MAX_SPHERE_RADIUS = 4.5f;

const float SPHERE_SCALE_TIME = 1.5f;
const float PRISM_SCALE_TIME  = 1.5f;
const float FADE_TIME         = 2.0f;
const float START_FADE_TIME   = 4.0f;

const float FIXED_FOV = 60.f;

const float X_SPEED    = PI / 2;
const float Z_SPEED    = -PI;
const float ANGLE_STEP = 360.f / 60.f;
const float ANGLES[]   = { PI / 2, PI, 0.f };

const double CAMERA_NEAR_PLANE = 0.1;
const double CAMERA_FAR_PLANE  = 100.0;

Camera camera;
Texture2D orbTexture;

/* Call from frontend using ccall/cwrap */
extern "C"
{
    bool is_visible = true;
    void set_visibility(bool visible)
    {
        is_visible = visible;
    }
}

float GetOrbRotationAngle(float time, int i) 
{
    return time * i * ANGLE_STEP * DEG2RAD;
}

float GetClockRotationAngle(float hour) 
{
    return hour < 12 ? hour * -30.0f : (hour - 12) * -30.0f;
}

float LerpXRotationAngle(ElapsedSeconds& s, Time& t) 
{
    return Lerp(ANGLES[t.minute % 3], ANGLES[(t.minute + 1) % 3], Normalize(s.minute, 0.f, 60.f));
}

Matrix GetRotationMatrix(ElapsedSeconds& s, Time& t, float hourAngle)
{
    float ax = X_SPEED * s.minute + LerpXRotationAngle(s, t);
    float az = Z_SPEED * s.minute;

    Matrix rx = MatrixRotateX(ax);
    Matrix rz = MatrixRotateZ(az);

    return MatrixMultiply(MatrixMultiply(rz, rx), MatrixRotateZ(hourAngle));
}

Vector3 GetOrbPosition(float time, float radius, int orbIndex, Matrix& rotation) 
{
    float angle = GetOrbRotationAngle(time, orbIndex);
    return Vector3Transform(
        { radius * cosf(angle), radius * sinf(angle), 0.f },
        rotation
    );
}

void DrawTrail(TimePoint prevTime, float radius, float hourAngle, int orbIndex)
{
    Time time;
    ElapsedSeconds seconds;

    Vector3 prevPosition;
    Matrix rotation;

    float deltaTime, elapsedSeconds;

    for (int j = 0; j < TRAIL_LENGTH; j++)
    {
        deltaTime = GetFrameTime();
        GetTimeInfo(&time, prevTime);
        GetElapsedSeconds(&seconds, time);

        elapsedSeconds = seconds.minute;
        rotation       = GetRotationMatrix(seconds, time, hourAngle);
        prevPosition   = GetOrbPosition(elapsedSeconds, radius, orbIndex, rotation);
        prevTime       -= chrono::milliseconds((int)(deltaTime * 1000.f));

        DrawBillboard(camera, orbTexture, prevPosition, 0.3f, Fade(WHITE, 0.7f));
    }
}

void DrawOrbs(Model& prism, ElapsedSeconds seconds, Time time, float radius)
{
    float elapsedHours   = fmod(seconds.day / 3600, 24);
    float elapsedSeconds = seconds.minute;
    float hourAngle      = (90.f + GetClockRotationAngle(elapsedHours)) * DEG2RAD;

    Vector3 orbPosition;
    Matrix rotation = GetRotationMatrix(seconds, time, hourAngle);

    for (int i = 0; i < ORBS; i++)
    {
        orbPosition = GetOrbPosition(elapsedSeconds, radius, i, rotation);
        SetShaderValue(
            prism.materials[0].shader, 
            GetShaderLocation(prism.materials[0].shader, TextFormat("pointLights[%d].position", i)), 
            &orbPosition, 
            RL_SHADER_UNIFORM_VEC3
        );

        DrawBillboard(camera, orbTexture, orbPosition, 2.f, WHITE);
        DrawTrail(time.timePoint, radius, hourAngle, i);
    }
}

void DrawClock(Model& prism, Vector3& clockPosition, 
               float secOfMinRotation, float hourOfDayRotation, float hourPrismScale)
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

float GetVerticalFOV()
{
    if (screenHeight > screenWidth) 
    {
        float aspect = (float)screenWidth / screenHeight;
        return 2.0f * atanf(tanf(FIXED_FOV * DEG2RAD * 0.5f) / aspect) * RAD2DEG;
    }
    return FIXED_FOV;
}

void InitCamera() 
{
    Vector3 position = { 0.0f, 0.0f, 30.f };
    Vector3 target   = { 0.0f, 0.0f, -1.f  };
    Vector3 up       = { 0.0f, 1.0f, 0.0f  };

    camera.position = position;
    camera.target   = target;
    camera.up       = up;

    camera.fovy = GetVerticalFOV();
    camera.projection = CAMERA_PERSPECTIVE;
}

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

float LerpClockRotation(float elapsedSecondsMinute) 
{
    return Lerp(0.f, 360.f, Normalize(elapsedSecondsMinute, 0.f, 60.f));
}

float LerpPrismScale(float elapsedSecondsHour)
{
    return 1.f - elapsedSecondsHour / 3600.f;
}

float LerpSphereRadius(float elapsedSecondsHour) 
{
    return Lerp(MIN_SPHERE_RADIUS, MAX_SPHERE_RADIUS, Normalize(elapsedSecondsHour, 0.f, 3600.f));
}

float InvLerpSphereRadius(float elapsedSeconds)
{
    return Lerp(MAX_SPHERE_RADIUS, MIN_SPHERE_RADIUS, Normalize(elapsedSeconds, 0.f, SPHERE_SCALE_TIME));
}

float InvLerpPrismScale(float elapsedSeconds)
{
    return Lerp(0.f, 1.f, Normalize(elapsedSeconds, 0.f, PRISM_SCALE_TIME));
}

void DrawDateTime(const Time& time, const char* timeLocale)
{
    string dateStr = FormatDate(time, timeLocale);
    string timeStr = FormatTime(time, timeLocale);

    int timeSize = MeasureText(timeStr.c_str(), 30);
    DrawText(dateStr.c_str(), 10, 10, 30, WHITE);
    DrawText(timeStr.c_str(), screenWidth - timeSize - 10, 10, 30, WHITE);
}
//------------------------------------------------------------------------------------
// Program main entry point
//------------------------------------------------------------------------------------
int main(int argc, char** argv)
{
    //------------------------------------------------------------------------------------
    // Parse command line parameters
    //------------------------------------------------------------------------------------

    Config cfg = { 0 };
    string err = "";

    if (!ParseCMD(cfg, argc, argv, err))
    {
        if (!ParseINI(cfg, "resources/config.ini"))
        {
            cerr << "Could not read config" << endl;
            return 1;
        }
    }

    screenWidth  = cfg.screenWidth;
    screenHeight = cfg.screenHeight;

    //------------------------------------------------------------------------------------
    // Retrieve time locale
    //------------------------------------------------------------------------------------

    setlocale(LC_ALL, ""); 
    const char* timeLocale = setlocale(LC_TIME, nullptr);

    //------------------------------------------------------------------------------------
    // Camera/window initialization
    //------------------------------------------------------------------------------------

    SetConfigFlags(cfg.flags | FLAG_WINDOW_RESIZABLE);
    InitWindow(screenWidth, screenHeight, "PS2 Clock");

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

    camera = { 0 };
    InitCamera();
    SetTargetFPS(60);

    //------------------------------------------------------------------------------------
    // Loading audio
    //------------------------------------------------------------------------------------

    InitAudioDevice();
    Music ambience   = LoadMusicStream("resources/sound/waves.ogg");
    ambience.looping = true;

    PlayMusicStream(ambience);

    //------------------------------------------------------------------------------------
    // Loading main models and textures
    //------------------------------------------------------------------------------------
    RenderTexture2D tunnelLayer = LoadRenderTexture(screenWidth, screenHeight);
    RenderTexture2D orbsLayer   = LoadRenderTexture(screenWidth, screenHeight);
    RenderTexture2D clockLayer  = LoadRenderTexture(screenWidth, screenHeight);

    Model prism = LoadModel("resources/prism.obj");
    Model tube  = LoadModelFromMesh(GenMeshCylinder(20.f, 100, 30));

    Texture2D normalTexture = LoadTexture("resources/textures/normal.jpg");
    Texture2D noiseTexture  = LoadTexture("resources/textures/noiseTexture.png");
    orbTexture = LoadTexture("resources/textures/halo.png");

    int normalMap = MATERIAL_MAP_NORMAL;
    GenMeshTangents(&(prism.meshes[0]));

    //------------------------------------------------------------------------------------
    // Shaders/materials
    //------------------------------------------------------------------------------------

    string glslDirectory = "resources/shaders/glsl" + to_string(GLSL_VERSION);
    Shader crystalShader = LoadShader((glslDirectory + "/crystal.vs").c_str(), (glslDirectory + "/crystal.fs").c_str());
    Shader tunnelShader  = LoadShader((glslDirectory + "/tunnel.vs").c_str(),  (glslDirectory + "/tunnel.fs").c_str());
    Shader orbShader     = LoadShader(NULL, (glslDirectory + "/orb.fs").c_str());

    //------------------------------------------------------------------------------------
    // Crystal rod
    //------------------------------------------------------------------------------------

    Vector3 crystalAmbient   = { 0.04, 0.23, 0.46 };
    Vector3 crystalDiffuse   = { 0.04, 0.23, 0.46 };
    Vector3 crystalSpecular  = { 1.0,  1.0,  1.0  };
    float   crystalShininess = 0.4;

    Vector3 orbLightAmbient   = { 0.5, 0.5, 0.5 };
    Vector3 orbLightDiffuse   = { 0.8, 0.8, 0.8 };
    Vector3 orbLightSpecular  = { 1.0, 1.0, 1.0 };

    Vector3 dirLightAmbient  = { 0.5, 0.5,  0.5  };
    Vector3 dirLightDiffuse  = { 0.8, 0.8,  0.8  };
    Vector3 dirLightSpecular = { 0.5, 0.5,  0.5  };
    Vector3 lightDirection   = { 0.0, 0.0,  -1.0 };

    SetShaderValue(crystalShader, GetShaderLocation(crystalShader, "material.ambient"),   &crystalAmbient,   RL_SHADER_UNIFORM_VEC3);
    SetShaderValue(crystalShader, GetShaderLocation(crystalShader, "material.diffuse"),   &crystalDiffuse,   RL_SHADER_UNIFORM_VEC3);
    SetShaderValue(crystalShader, GetShaderLocation(crystalShader, "material.specular"),  &crystalSpecular,  RL_SHADER_UNIFORM_VEC3);
    SetShaderValue(crystalShader, GetShaderLocation(crystalShader, "material.shininess"), &crystalShininess, RL_SHADER_UNIFORM_FLOAT);

    SetShaderValue(crystalShader, GetShaderLocation(crystalShader, "dirLight.direction"), &lightDirection,    RL_SHADER_UNIFORM_VEC3);
    SetShaderValue(crystalShader, GetShaderLocation(crystalShader, "dirLight.ambient"),   &dirLightAmbient,   RL_SHADER_UNIFORM_VEC3);
    SetShaderValue(crystalShader, GetShaderLocation(crystalShader, "dirLight.diffuse"),   &dirLightDiffuse,   RL_SHADER_UNIFORM_VEC3);
    SetShaderValue(crystalShader, GetShaderLocation(crystalShader, "dirLight.specular"),  &dirLightSpecular,  RL_SHADER_UNIFORM_VEC3);

    // 13	1.0	0.35	0.44
    const float ORB_LIGHT_KC = 1.0;
    const float ORB_LIGHT_KL = 0.7;
    const float ORB_LIGHT_KQ = 1.8;

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

    // 1.0	0.007	0.0002
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

    //------------------------------------------------------------------------------------
    // Input variables
    //------------------------------------------------------------------------------------
    Time time;
    ElapsedSeconds elapsedSeconds;

    GetTimeInfo(&time);
    GetElapsedSeconds(&elapsedSeconds, time);

    Vector3 clockPosition  = { 0.0f, 5.0f, 0.0f };
    Vector3 prismColor     = LerpPrismColor(elapsedSeconds.minute);

    float elapsedTime;
    float secondsInMinute = elapsedSeconds.minute;
    float secondsInHour   = elapsedSeconds.hour;
    float secondsInDay    = elapsedSeconds.day;

    float clockMinuteRotation = LerpClockRotation(secondsInMinute);
    float clockHourRotation   = GetClockRotationAngle(time.hour);

    float sphereRadius        = LerpSphereRadius(secondsInHour);
    float prismScale          = LerpPrismScale(secondsInHour);

    float sphereRadiusAnim = 0.f;
    float prismScaleAnim   = 0.f;
    float fadeAnim         = 0.f;

    bool newHour;
    bool fading;

    bool showClock = true;
    bool showTime  = true;

    Color clockLayerTint = BLACK;
    Color orbLayerTint   = BLACK;

    //------------------------------------------------------------------------------------
    // Setting render options
    //------------------------------------------------------------------------------------
    rlEnableColorBlend();
    rlEnableSmoothLines();

    rlDisableBackfaceCulling();

    rlSetClipPlanes(CAMERA_NEAR_PLANE, CAMERA_FAR_PLANE);
    rlSetLineWidth(2.f);

    Matrix M, N;
    M = MatrixMultiply(MatrixRotateX(PI + PI / 2), MatrixTranslate(0.f, 0.f, 30.f));
    N = MatrixInvert(MatrixTranspose(M));

    while (!WindowShouldClose())
    {
        //------------------------------------------------------------------------------------
        // Update
        //------------------------------------------------------------------------------------
        UpdateMusicStream(ambience);
        if (IsWindowHidden() || IsWindowMinimized() || !is_visible)
        {
            continue;
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
            camera.fovy = GetVerticalFOV();
        }

        GetTimeInfo(&time);
        GetElapsedSeconds(&elapsedSeconds, time);

        elapsedTime     = GetTime();
        secondsInMinute = elapsedSeconds.minute;
        secondsInHour   = elapsedSeconds.hour;
        secondsInDay    = elapsedSeconds.day;

        prismColor          = LerpPrismColor(elapsedSeconds.minute);
        clockMinuteRotation = LerpClockRotation(secondsInMinute);
        clockHourRotation   = GetClockRotationAngle(time.hour);
        newHour             = (int)roundf(secondsInHour) == 0 || sphereRadiusAnim > 0.f || prismScaleAnim > 0.f;

        //------------------------------------------------------------------------------------
        // Controls
        //------------------------------------------------------------------------------------
        fading = IsKeyPressed(KEY_J) || fadeAnim > 0.f;
        if (IsKeyPressed(KEY_K))
            showTime = !showTime;

        //------------------------------------------------------------------------------------
        // Animations
        //------------------------------------------------------------------------------------
        if (elapsedTime < START_FADE_TIME)
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
            float deltaTime = GetFrameTime();
            Vector3 color   = Vector3Lerp(
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
            float deltaTime = GetFrameTime();
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

        //------------------------------------------------------------------------------------
        // Set shader uniforms
        //------------------------------------------------------------------------------------
        if (showClock || fading)
        {
            SetShaderValue(tube.materials[0].shader, GetShaderLocation(tube.materials[0].shader, "time"), &elapsedTime, SHADER_UNIFORM_FLOAT);

            SetShaderValueMatrix(tube.materials[0].shader, GetShaderLocation(tube.materials[0].shader, "model"),   M);
            SetShaderValueMatrix(tube.materials[0].shader, GetShaderLocation(tube.materials[0].shader, "mNormal"), N);

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
                        rlMultMatrixf(MatrixToFloat(M));
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
                DrawOrbs(prism, elapsedSeconds, time, sphereRadius);
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
                    DrawClock(prism, clockPosition, clockMinuteRotation, clockHourRotation, prismScale);
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
                DrawTextureRec(clockLayer.texture, { 0, 0, (float)screenWidth, (float) -screenHeight}, {0, 0}, clockLayerTint);

                if (showTime)
                    DrawDateTime(time, timeLocale);
            }
            DrawTextureRec(orbsLayer.texture,  { 0, 0, (float)screenWidth, (float) -screenHeight}, {0, 0}, orbLayerTint);
        EndDrawing();
    }

    // De-Initialization
    //--------------------------------------------------------------------------------------
    StopMusicStream(ambience);
    UnloadMusicStream(ambience);

    UnloadTexture(orbTexture);
    UnloadTexture(noiseTexture);
    UnloadTexture(normalTexture);

    UnloadShader(crystalShader);
    UnloadShader(orbShader);
    UnloadShader(tunnelShader);

    UnloadModel(prism);
    UnloadModel(tube);

    UnloadRenderTexture(tunnelLayer);
    UnloadRenderTexture(orbsLayer);
    UnloadRenderTexture(clockLayer);

    CloseWindow();
    return 0;
}
#ifndef CLOCK_H
#define CLOCK_H

bool Initialize(int argc, char** argv);
void Uninitialize();

void InitWindow();
void InitCamera();
void DestroyWindow();

void LoadResources();
void UnloadResources();

void Loop();

void SetRenderOptions();
void SetWindowResolution(int width, int height);
void SetResourcesPath(const char* path);
void SetTimeLocale();
void SetTextSize(int px);

#endif
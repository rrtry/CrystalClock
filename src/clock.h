#ifndef CLOCK_H
#define CLOCK_H

bool Initialize(int argc, char** argv);
void Uninitialize();

void InitWindow();
void DestroyWindow();

void LoadResources();
void UnloadResources();

void Loop();
void SetRenderOptions();

#endif
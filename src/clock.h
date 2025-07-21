#ifndef CLOCK_H
#define CLOCK_H

void Initialize(int argc, char** argv);
void Initialize();
bool ParseConfig(int argc, char** argv, bool prefsOnly);

void Uninitialize();
void Loop();

void SetWindowResolution(int width, int height);
void SetTextSize(int px);

void SetFadeIn(bool fade);
void SetPlaySound(bool play);
void SetShowTime(bool show);

#endif
#include "RaylibDesktop.h"
#include "raylib.h"
#include "../clock.h"

extern int GetTargetDisplay();

int main(int argc, char** argv)
{
	// Parse config
	if (!ParseConfig(argc, argv, true)) // prefsOnly: Parse only preference flags (-nosound -nofadein)
		return 1;

	// Initializes desktop replacement magic
	InitRaylibDesktop();

	// Sets up the desktop (-1 is the entire desktop spanning all monitors)
	MonitorInfo monitorInfo = GetWallpaperTarget(GetTargetDisplay());

	// Set resolution
	SetWindowResolution(monitorInfo.rcWorkWidth, monitorInfo.rcWorkHeight);

	// Initialize
	SetShowTime(false);
	Initialize();

	// Retrieve the handle for the raylib-created window.
	void *raylibWindowHandle = GetWindowHandle();

	// Reparent the raylib window to the window behind the desktop icons.
	RaylibDesktopReparentWindow(raylibWindowHandle);

	// Configure the desktop positioning.
	ConfigureDesktopPositioning(monitorInfo);

	// Main render loop.
	while (!WindowShouldClose()) 
	{
		// skip rendering if the wallpaper is occluded more than 95%
		if (IsMonitorOccluded(monitorInfo, 0.95)) 
		{
			WaitTime(0.1);
			continue;
		}
		Loop();
	}

	Uninitialize();
	// Clean up the desktop window.
	// Restores the original wallpaper.
	CleanupRaylibDesktop();
	return 0;
}

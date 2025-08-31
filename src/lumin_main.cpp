#include "raylib.h"
#include "lumin.h"
#include "clock.h"

extern int GetTargetDisplay();

int main(int argc, char** argv)
{
	// Parse config
	if (!ParseConfig(argc, argv, true)) // prefsOnly: Parse only preference flags (-nosound -nofadein)
		return 1;
	
	// Initializes desktop replacement magic
	lumin::Initialize();

	// Sets up the desktop (-1 is the entire desktop spanning all monitors)
	lumin::MonitorInfo monitorInfo = lumin::GetWallpaperTarget(GetTargetDisplay());

	// Set resolution
	SetWindowResolution(monitorInfo.workWidth, monitorInfo.workHeight);

	// Initialize
	SetShowTime(false);
	Initialize();

	// Retrieve the handle for the raylib-created window.
	void *raylibWindowHandle = GetWindowHandle();

	// Reparent the raylib window to the window behind the desktop icons.
	lumin::ConfigureWallpaperWindow(raylibWindowHandle, monitorInfo);

	// Now, enter the raylib render loop.
	SetTargetFPS(60);

	// Main render loop.
	while (!WindowShouldClose()) 
	{

		// skip rendering if the wallpaper is occluded more than 95%
		if (lumin::IsMonitorOccluded(monitorInfo, 0.95)) 
		{
			WaitTime(0.1);
			continue;
		}
		if (lumin::IsDesktopLocked() ) 
		{
			WaitTime(0.1);
			continue;
		}
		Loop();
	}

	Uninitialize();
	lumin::Cleanup();
	return 0;
}

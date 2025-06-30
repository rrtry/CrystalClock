#pragma once
#include <vector>

// Call this function to initialize the desktop window.
int InitRaylibDesktop();

// Monitor setup
// Structure to hold information about a monitor
typedef struct MonitorInfo
{
	int monitorLeftCoordinate; // X coordinate of the monitor's top-left corner
	int monitorTopCoordinate; // Y coordinate of the monitor's top-left corner
	int monitorWidth; // Monitor width in pixels
	int monitorHeight; // Monitor height in pixels
};

// Enumerate all monitors and return their information
std::vector<MonitorInfo> EnumerateAllMonitors();

// pass -1 to get the entire desktop
MonitorInfo GetWallpaperTarget(int monitorIndex);

// Configure Desktop Positioning
void ConfigureDesktopPositioning(MonitorInfo monitorInfo);

// Monitor Occlusion Detection
bool IsMonitorOccluded(const MonitorInfo &monitor, double occlusionThreshold = 0.95);

// Call this function to reparent the raylib window to the desktop after raylib has created its own.
void RaylibDesktopReparentWindow(void *raylibWindowHandle);

// Call this function to clean up the desktop window.
void CleanupRaylibDesktop();


// Mouse replacements since raylib's mouse functions don't work with custom parent windows.
// Minimal forward declaration of a Vector2 type (avoid conflicts with raylib's version)
class Vector2;

// Call this function once per frame to update mouse states.
// It updates both the previous and current state arrays.
void RaylibDesktopUpdateMouseState(void);

// Mouse button state functions.
// The valid button indices are:
//   0: Left button (VK_LBUTTON)
//   1: Right button (VK_RBUTTON)
//   2: Middle button (VK_MBUTTON)
//   3: XButton1   (VK_XBUTTON1)
//   4: XButton2   (VK_XBUTTON2)
bool RaylibDesktopIsMouseButtonPressed(int button); // Returns true only on the frame the button was pressed.
bool RaylibDesktopIsMouseButtonDown(int button); // Returns true if the button is currently down.
bool RaylibDesktopIsMouseButtonReleased(int button); // Returns true only on the frame the button was released.
bool RaylibDesktopIsMouseButtonUp(int button); // Returns true if the button is currently up.

// Mouse position functions.
// These return the global cursor position (in physical pixels).
int RaylibDesktopGetMouseX(void);
int RaylibDesktopGetMouseY(void);
Vector2 RaylibDesktopGetMousePosition(void);

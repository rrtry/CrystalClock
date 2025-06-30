#include "RaylibDesktop.h"

#include <Windows.h>
#include <limits>

// For occlusion detection
#include <dwmapi.h>
// Required for DwmGetWindowAttribute
#pragma comment(lib, "Dwmapi.lib")

// For DPI awareness functions
#include <shellscalingapi.h>
// Required for SetProcessDpiAwareness and GetDpiForMonitor
#pragma comment(lib, "Shcore.lib")

// Global variables to hold handles within the desktop hierarchy
// g_progmanWindowHandle : top level Program Manager window
// g_workerWindowHandle  : child WorkerW window rendering the static wallpaper
// g_shellViewWindowHandle: child ListView window displaying the desktop icons
// g_raylibWindowHandle  : handle to the raylib window we inject
HWND g_progmanWindowHandle = NULL;
HWND g_workerWindowHandle = NULL;
HWND g_shellViewWindowHandle = NULL;
HWND g_raylibWindowHandle = NULL;

// current monitor in desktop coordinates
MonitorInfo g_selectedMonitor = {0, 0, 0, 0};

// the offset to the desktop coordinates
// windows desktop coordinates start at the top left of the primary monitor
// subtract this offset to get the desktop coordinates
int g_desktopX = 0;
int g_desktopY = 0;

// Monitor enumeration
// Callback function called for each monitor by EnumDisplayMonitors
BOOL CALLBACK MonitorEnumProc(
	HMONITOR monitorHandle, // Handle to the display monitor
	HDC monitorDeviceContext, // Handle to a device context (not used here)
	LPRECT monitorRectangle, // Pointer to a RECT structure (not used directly)
	LPARAM lParam // Application-defined data; here, a pointer to a vector<MonitorInfo>
)
{
	// Cast lParam to a pointer to our vector of MonitorInfo
	std::vector<MonitorInfo> *pointerToMonitorVector = reinterpret_cast<std::vector<MonitorInfo> *>(lParam);

	// Prepare a MONITORINFOEX structure to receive monitor information
	MONITORINFOEX monitorInfoEx;
	monitorInfoEx.cbSize = sizeof(MONITORINFOEX);

	// Retrieve monitor information (including coordinates and dimensions)
	if (GetMonitorInfo(monitorHandle, &monitorInfoEx)) {
		// Calculate monitor coordinates and dimensions
		int leftCoordinate = monitorInfoEx.rcMonitor.left; // Left X coordinate
		int topCoordinate = monitorInfoEx.rcMonitor.top; // Top Y coordinate
		int widthOfMonitor = monitorInfoEx.rcMonitor.right - monitorInfoEx.rcMonitor.left; // Width = right - left
		int heightOfMonitor = monitorInfoEx.rcMonitor.bottom - monitorInfoEx.rcMonitor.top; // Height = bottom - top

		// Create a MonitorInfo object with the retrieved data
		MonitorInfo currentMonitorInfo;
		currentMonitorInfo.monitorLeftCoordinate = leftCoordinate;
		currentMonitorInfo.monitorTopCoordinate = topCoordinate;
		currentMonitorInfo.monitorWidth = widthOfMonitor;
		currentMonitorInfo.monitorHeight = heightOfMonitor;

		// Add the monitor information to the vector
		pointerToMonitorVector->push_back(currentMonitorInfo);
	}

	// Returning TRUE tells EnumDisplayMonitors to continue the enumeration.
	return TRUE;
}

// Function to enumerate all monitors and return their information
std::vector<MonitorInfo> EnumerateAllMonitors()
{
	// Vector to store information about each monitor
	std::vector<MonitorInfo> monitorInfoVector;

	// Call EnumDisplayMonitors.
	// The first two parameters are NULL to indicate the entire virtual screen.
	// The callback MonitorEnumProc will be called for each monitor.
	EnumDisplayMonitors(NULL, NULL, MonitorEnumProc, reinterpret_cast<LPARAM>(&monitorInfoVector));

	// convert to desktop coordinates which start at 0,0
	g_desktopX = INT_MAX;
	g_desktopY = INT_MAX;

	for (auto &monitor : monitorInfoVector) {
		if (monitor.monitorLeftCoordinate < g_desktopX) {
			g_desktopX = monitor.monitorLeftCoordinate;
		}
		if (monitor.monitorTopCoordinate < g_desktopY) {
			g_desktopY = monitor.monitorTopCoordinate;
		}
	}

	for (auto &monitor : monitorInfoVector) {
		monitor.monitorLeftCoordinate -= g_desktopX;
		monitor.monitorTopCoordinate -= g_desktopY;
	}

	return monitorInfoVector;
}

MonitorInfo GetWallpaperTarget(int monitorIndex)
{
	// If monitorIndex is -1, then we use the entire virtual desktop.
	std::vector<MonitorInfo> monitors = EnumerateAllMonitors();

	if (monitorIndex < 0 || monitorIndex >= static_cast<int>(monitors.size())) {
		MonitorInfo info;
		info.monitorLeftCoordinate = 0; // GetSystemMetrics(SM_XVIRTUALSCREEN);
		info.monitorTopCoordinate = 0; // GetSystemMetrics(SM_YVIRTUALSCREEN);
		info.monitorWidth = GetSystemMetrics(SM_CXVIRTUALSCREEN);
		info.monitorHeight = GetSystemMetrics(SM_CYVIRTUALSCREEN);
		return info;
	}
	else {
		// Otherwise, try to get the desired monitor from the enumeration.
		return monitors[monitorIndex];
	}
}

// Wallpaper Occlusion Fix
// Data structure to hold parameters for occlusion detection via EnumWindows.
struct FullscreenOcclusionData
{
	MonitorInfo monitor; // Target monitor area (already adjusted relative to (0,0))
	std::vector<RECT> occludedRects; // Rectangles of occluded areas
};

static bool IsInvisibleWin10BackgroundAppWindow(HWND hWnd)
{
	int CloakedVal;
	HRESULT hRes = DwmGetWindowAttribute(hWnd, DWMWA_CLOAKED, &CloakedVal, sizeof(CloakedVal));
	if (hRes != S_OK) {
		CloakedVal = 0;
	}
	return CloakedVal ? true : false;
}

// @brief Computes the fraction of the monitor area that is occluded by any rectangle in occludedRects.
// @param occludedRects A vector of RECTs representing occluded regions.
// @param monitor The monitor info (with coordinates relative to your desktop, starting at (0,0)).
// @param sampleStep The spacing (in pixels) between sample points on the grid.
// @return A value between 0.0 and 1.0 representing the approximate fraction of the monitor area that is occluded.
double
ComputeOcclusionFraction(const std::vector<RECT> &occludedRects, const MonitorInfo &monitor, int sampleStep = 100)
{
	int occludedCount = 0;
	int totalSamples = 0;

	// Loop over the monitor's area using the given step size.
	for (int y = monitor.monitorTopCoordinate; y < monitor.monitorTopCoordinate + monitor.monitorHeight;
		 y += sampleStep) {
		for (int x = monitor.monitorLeftCoordinate; x < monitor.monitorLeftCoordinate + monitor.monitorWidth;
			 x += sampleStep) {
			totalSamples++;
			bool isOccluded = false;
			// Check if this sample point is within any occlusion rectangle.
			for (const RECT &rect : occludedRects) {
				if (x >= rect.left && x < rect.right && y >= rect.top && y < rect.bottom) {
					isOccluded = true;
					break;
				}
			}
			if (isOccluded)
				occludedCount++;
		}
	}

	// Avoid division by zero.
	if (totalSamples == 0)
		return 0.0;

	return static_cast<double>(occludedCount) / static_cast<double>(totalSamples);
}

// struct WindowDebug
//{
//	HWND hwnd;
//     char g_szClassName[256];
//
//	RECT rect;
//	double occludedFraction;
// };
//
// std::vector<WindowDebug> g_debugWindows = {};

// Callback function for EnumWindows. This is called for each top-level window.
BOOL CALLBACK FullscreenWindowEnumProc(HWND hwnd, LPARAM lParam)
{
	FullscreenOcclusionData *occlusionData = reinterpret_cast<FullscreenOcclusionData *>(lParam);

	if (hwnd == g_raylibWindowHandle || hwnd == g_workerWindowHandle) {
		return TRUE;
	}

	// Skip non-visible or minimized windows.
	if (!IsWindowVisible(hwnd) || IsIconic(hwnd)) {
		return TRUE;
	}

	// make sure it isnt the shell window
	if (GetShellWindow() == hwnd) {
		return TRUE;
	}

	// make sure it isnt a workerw window
	char g_szClassName[256];
	GetClassNameA(hwnd, g_szClassName, 256);

	if (strcmp(g_szClassName, "WorkerW") == 0) {
		return TRUE;
	}

	// check that it isnt the Nvidia overlay
	if (strcmp(g_szClassName, "CEF-OSC-WIDGET") == 0) {
		return TRUE;
	}

	// Skip the invisible windows that are part of the Windows 10 background app
	if (IsInvisibleWin10BackgroundAppWindow(hwnd)) {
		return TRUE;
	}

	// Retrieve the window's bounding rectangle.
	RECT windowRect;
	if (!GetWindowRect(hwnd, &windowRect))
		return TRUE;

	// convert window rect to desktop coordinates
	windowRect.left -= g_desktopX;
	windowRect.right -= g_desktopX;
	windowRect.top -= g_desktopY;
	windowRect.bottom -= g_desktopY;

	// Build a rectangle for the target monitor.
	RECT monitorRect;
	monitorRect.left = occlusionData->monitor.monitorLeftCoordinate;
	monitorRect.top = occlusionData->monitor.monitorTopCoordinate;
	monitorRect.right = occlusionData->monitor.monitorLeftCoordinate + occlusionData->monitor.monitorWidth;
	monitorRect.bottom = occlusionData->monitor.monitorTopCoordinate + occlusionData->monitor.monitorHeight;

	// Calculate the intersection of the window's rectangle with the monitor's rectangle.
	RECT intersectionRect;
	if (!IntersectRect(&intersectionRect, &windowRect, &monitorRect)) {
		// No intersection at all.
		return TRUE;
	}

	// store the occluded area
	occlusionData->occludedRects.push_back(intersectionRect);

	// double occludedFraction = ComputeOcclusionFraction(occlusionData->occludedRects, occlusionData->monitor);

	// WindowDebug debugWindow;
	// debugWindow.hwnd = hwnd;
	// debugWindow.occludedFraction = occludedFraction;
	// debugWindow.rect = intersectionRect;
	// strcpy_s(debugWindow.g_szClassName, g_szClassName);

	// g_debugWindows.push_back(debugWindow);

	// if (occludedFraction >= 0.95)
	//{
	//	// Stop enumeration if the monitor is mostly occluded.
	//	return FALSE;
	// }

	// Continue checking other windows.
	return TRUE;
}

// Determines whether any fullscreen (or large) window occludes the given monitor area.
// The monitor's coordinates should be relative to the desktop origin (i.e., (0,0) at the top-left).
// The occlusionThreshold parameter specifies what fraction of the monitor must be covered
// to consider it occluded (e.g., 0.9 means 90%).
//
// Returns: true if the monitor is occluded; false otherwise.
bool IsMonitorOccluded(const MonitorInfo &monitor, double occlusionThreshold)
{
	FullscreenOcclusionData occlusionData;
	occlusionData.monitor = monitor;
	occlusionData.occludedRects = {};

	// g_debugWindows = {};

	// Enumerate all top-level windows.
	EnumWindows(FullscreenWindowEnumProc, reinterpret_cast<LPARAM>(&occlusionData));

	// Calculate the fraction of the monitor that is occluded.
	double occludedFraction = ComputeOcclusionFraction(occlusionData.occludedRects, monitor);

	// Return true if the occluded fraction exceeds the threshold.
	return occludedFraction >= occlusionThreshold;
}

// Callback function for EnumWindows to locate the proper WorkerW window
BOOL CALLBACK EnumWindowsProc(HWND windowHandle, LPARAM lParam)
{
	// Look for a child window named "SHELLDLL_DefView" in each top-level window.
	HWND shellViewWindow = FindWindowEx(windowHandle, NULL, L"SHELLDLL_DefView", NULL);
	if (shellViewWindow != NULL) {
		// If found, get the WorkerW window that is a sibling of the found window.
		g_workerWindowHandle = FindWindowEx(NULL, windowHandle, L"WorkerW", NULL);
		return FALSE; // Stop enumeration since we have found the desired window.
	}
	return TRUE;
}

int InitRaylibDesktop()
{
	// Set the process DPI awareness to get physical pixel coordinates.
	// This must be done before any windows are created.
	HRESULT dpiAwarenessResult = SetProcessDpiAwareness(PROCESS_PER_MONITOR_DPI_AWARE);
	if (FAILED(dpiAwarenessResult)) {
		MessageBox(NULL, L"Failed to set DPI awareness.", L"Error", MB_OK);
		// Continue if needed, but coordinate values may be scaled.
	}

	// Locate the Progman window (the desktop window)
	g_progmanWindowHandle = FindWindow(L"Progman", NULL);

	// Send message 0x052C to Progman to force creation of a WorkerW window
	LRESULT result = 0;
	SendMessageTimeout(
		g_progmanWindowHandle,
		0x052C, // Undocumented message to trigger WorkerW creation
		0,
		0,
		SMTO_NORMAL,
		1000,
		reinterpret_cast<PDWORD_PTR>(&result)
	);

	// Try to locate the Shell view (desktop icons) and WorkerW child directly under Progman
	g_shellViewWindowHandle = FindWindowEx(g_progmanWindowHandle, NULL, L"SHELLDLL_DefView", NULL);
	g_workerWindowHandle = FindWindowEx(g_progmanWindowHandle, NULL, L"WorkerW", NULL);

	// Fallback for pre-24H2 builds where the WorkerW is a sibling window
	if (g_workerWindowHandle == NULL) {
		EnumWindows(EnumWindowsProc, 0);
	}

	if (g_workerWindowHandle == NULL) {
		MessageBox(NULL, L"Failed to find WorkerW window.", L"Error", MB_OK);
		return -1;
	}

	return 0;
}

void RaylibDesktopReparentWindow(void *raylibWindowHandle)
{
	g_raylibWindowHandle = (HWND)raylibWindowHandle;

	// Prepare the raylib window to be a layered child of Progman
	LONG_PTR style = GetWindowLongPtr(g_raylibWindowHandle, GWL_STYLE);
	style &= ~(WS_OVERLAPPEDWINDOW); // Remove decorations
	style |= WS_CHILD; // Child style required for SetParent
	SetWindowLongPtr(g_raylibWindowHandle, GWL_STYLE, style);

	LONG_PTR exStyle = GetWindowLongPtr(g_raylibWindowHandle, GWL_EXSTYLE);
	exStyle |= WS_EX_LAYERED; // Make it a layered window for 24H2
	SetWindowLongPtr(g_raylibWindowHandle, GWL_EXSTYLE, exStyle);
	SetLayeredWindowAttributes(g_raylibWindowHandle, 0, 255, LWA_ALPHA);

	// Reparent the raylib window directly to Progman
	SetParent(g_raylibWindowHandle, g_progmanWindowHandle);

	// Ensure correct Z-order: below icons but above the system wallpaper
	if (g_shellViewWindowHandle) {
		SetWindowPos(
			g_raylibWindowHandle, g_shellViewWindowHandle, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE
		);
	}
	if (g_workerWindowHandle) {
		SetWindowPos(g_workerWindowHandle, g_raylibWindowHandle, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE);
	}

	RedrawWindow(g_raylibWindowHandle, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW);
}

void ConfigureDesktopPositioning(MonitorInfo monitorInfo)
{
	g_selectedMonitor = monitorInfo;

	// SetWindowPos(g_raylibWindowHandle, NULL, 0, 0, monitorInfo.monitorWidth, monitorInfo.monitorHeight, SWP_NOZORDER
	// | SWP_NOACTIVATE);

	// Resize/reposition the raylib window to match its new parent.
	// g_progmanWindowHandle spans the entire virtual desktop in modern builds
	SetWindowPos(
		g_raylibWindowHandle,
		NULL,
		monitorInfo.monitorLeftCoordinate,
		monitorInfo.monitorTopCoordinate,
		monitorInfo.monitorWidth,
		monitorInfo.monitorHeight,
		SWP_NOZORDER | SWP_NOACTIVATE
	);
}

void CleanupRaylibDesktop()
{
	wchar_t wallpaperPath[MAX_PATH] = {0};
	// Retrieve the current wallpaper path
	if (SystemParametersInfo(SPI_GETDESKWALLPAPER, MAX_PATH, wallpaperPath, 0)) {
		// Reapply the wallpaper to force a refresh.
		SystemParametersInfo(SPI_SETDESKWALLPAPER, 0, wallpaperPath, SPIF_UPDATEINIFILE | SPIF_SENDCHANGE);
	}
}

// Mouse replacement

// Avoid including raylib.h due to windows.h conflicts
typedef struct Vector2
{
	float x;
	float y;
} Vector2;

// We support 5 mouse buttons.
#define MOUSE_BUTTON_COUNT 5

// Arrays to hold the state of each mouse button.
// prevMouseState[] holds the state from the previous frame,
// currMouseState[] holds the state for the current frame.
static bool prevMouseState[MOUSE_BUTTON_COUNT] = {false, false, false, false, false};
static bool currMouseState[MOUSE_BUTTON_COUNT] = {false, false, false, false, false};

// Helper function: maps a button index to the corresponding virtual key.
static int GetVirtualKeyForMouseButton(int button)
{
	switch (button) {
	case 0:
		return VK_LBUTTON; // Left button
	case 1:
		return VK_RBUTTON; // Right button
	case 2:
		return VK_MBUTTON; // Middle button
	case 3:
		return VK_XBUTTON1; // XButton1
	case 4:
		return VK_XBUTTON2; // XButton2
	default:
		return 0; // Invalid button index
	}
}

// UpdateMouseState() should be called once per frame.
// It copies the current state into previous state, then queries the current state.
void RaylibDesktopUpdateMouseState(void)
{
	for (int i = 0; i < MOUSE_BUTTON_COUNT; i++) {
		prevMouseState[i] = currMouseState[i];
		int vk = GetVirtualKeyForMouseButton(i);
		if (vk != 0) {
			// GetAsyncKeyState returns a SHORT. The high-order bit is set if the key is currently down.
			currMouseState[i] = ((GetAsyncKeyState(vk) & 0x8000) != 0);
		}
		else {
			currMouseState[i] = false;
		}
	}
}

// Returns true if the mouse button was pressed this frame (down now but was up previously)
bool RaylibDesktopIsMouseButtonPressed(int button)
{
	if (button < 0 || button >= MOUSE_BUTTON_COUNT)
		return false;
	return (currMouseState[button] && !prevMouseState[button]);
}

// Returns true if the mouse button is currently held down
bool RaylibDesktopIsMouseButtonDown(int button)
{
	if (button < 0 || button >= MOUSE_BUTTON_COUNT)
		return false;
	return currMouseState[button];
}

// Returns true if the mouse button was released this frame (up now but was down previously)
bool RaylibDesktopIsMouseButtonReleased(int button)
{
	if (button < 0 || button >= MOUSE_BUTTON_COUNT)
		return false;
	return (!currMouseState[button] && prevMouseState[button]);
}

// Returns true if the mouse button is currently up
bool RaylibDesktopIsMouseButtonUp(int button)
{
	if (button < 0 || button >= MOUSE_BUTTON_COUNT)
		return false;
	return !currMouseState[button];
}

bool GetRelativeCursorPos(POINT *p)
{
	int selectedMonitorX = g_selectedMonitor.monitorLeftCoordinate;
	int selectedMonitorY = g_selectedMonitor.monitorTopCoordinate;

	if (GetCursorPos(p)) {
		// Convert to desktop coordinates
		p->x -= g_desktopX;
		p->y -= g_desktopY;

		// Convert to window coordinates
		p->x -= selectedMonitorX;
		p->y -= selectedMonitorY;
		return true;
	}

	return false;
}

// GetMouseX() and GetMouseY() return the global cursor position in physical pixels.
int RaylibDesktopGetMouseX(void)
{
	POINT p;
	if (GetRelativeCursorPos(&p))
		return p.x;
	return 0;
}

int RaylibDesktopGetMouseY(void)
{
	POINT p;
	if (GetRelativeCursorPos(&p))
		return p.y;
	return 0;
}

// GetMousePosition() returns a Vector2 with the cursor's x and y coordinates.
Vector2 RaylibDesktopGetMousePosition(void)
{
	POINT p;
	Vector2 pos = {0.0f, 0.0f};
	if (GetRelativeCursorPos(&p)) {
		pos.x = (float)p.x;
		pos.y = (float)p.y;
	}
	return pos;
}

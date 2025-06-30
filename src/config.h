#ifndef CONFIG_H
#define CONFIG_H

#include <string>

constexpr auto FLAG_NO_SOUND   = 1 << 0;
constexpr auto FLAG_NO_FADE_IN = 1 << 1;

constexpr auto CMD_WIDTH  = "-width";
constexpr auto CMD_HEIGHT = "-height";

constexpr auto CMD_WIDTH_SHORT  = "-w";
constexpr auto CMD_HEIGHT_SHORT = "-h";

constexpr auto CMD_DISPLAY = "-display";
constexpr auto CMD_DISPLAY_SHORT = "-d";

constexpr auto CMD_BORDERLESS  = "-borderless";
constexpr auto CMD_FULLSCREEN  = "-fullscreen";
constexpr auto CMD_UNDECORATED = "-undecorated";
constexpr auto CMD_NO_FADE_IN  = "-nofadein";
constexpr auto CMD_NO_SOUND	   = "-nosound";

struct Config
{
	int screenWidth;
	int screenHeight;
	int display;
	int flags;
	int preferenceFlags;
};

enum Argument
{
	WIDTH,
	HEIGHT,

	BORDERLESS,
	FULLSCREEN,
	UNDECORATED,

	DISPLAY,
	NO_SOUND,
	NO_FADE_IN
};

struct CMDParameter
{
	Argument argument;
	bool hasValue;
};

bool ParseCMD(Config& cfg, int argc, char** argv, std::string& err, bool prefsOnly);
bool ParseINI(Config& cfg, const std::string& path, bool prefsOnly);

#endif


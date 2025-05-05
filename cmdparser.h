#ifndef CMDPARSER_H
#define CMDPARSER_H

#include <string>

constexpr auto CMD_WIDTH  = "-width";
constexpr auto CMD_HEIGHT = "-height";

constexpr auto CMD_SHORT_WIDTH  = "-w";
constexpr auto CMD_SHORT_HEIGHT = "-h";

constexpr auto CMD_BORDERLESS  = "-borderless";
constexpr auto CMD_FULLSCREEN  = "-fullscreen";
constexpr auto CMD_UNDECORATED = "-undecorated";

struct Config
{
	int screenWidth;
	int screenHeight;
	int flags;
};

enum Argument
{
	WIDTH,
	HEIGHT,

	BORDERLESS,
	FULLSCREEN,
	UNDECORATED
};

struct CMDParameter
{
	Argument argument;
	bool hasValue;
};

bool ParseCMD(Config& cfg, int argc, char** argv, std::string& err);
#endif


#include "raylib.h"
#include "cmdparser.h"

#include <map>

using namespace std;

map<Argument, int> flagsMap = {
	{ BORDERLESS,  FLAG_BORDERLESS_WINDOWED_MODE },
	{ FULLSCREEN,  FLAG_FULLSCREEN_MODE			 },
	{ UNDECORATED, FLAG_WINDOW_UNDECORATED		 }
};

map<string, CMDParameter> argsMap = {

	{ CMD_WIDTH,  { WIDTH,  true }},
	{ CMD_HEIGHT, { HEIGHT, true }},

	{ CMD_SHORT_WIDTH,  { WIDTH,  true }},
	{ CMD_SHORT_HEIGHT, { HEIGHT, true }},

	{ CMD_BORDERLESS,  { BORDERLESS,  false }},
	{ CMD_FULLSCREEN,  { FULLSCREEN,  false }},
	{ CMD_UNDECORATED, { UNDECORATED, false }}
};

int ParseInt(const string& cmd)
{
	try 
	{
		return stoi(cmd);
	}
	catch (exception& err)
	{
		return -1;
	}
}

bool ParseCMD(Config& config, int argc, char** argv, string& err)
{
	if (argc < 5)
	{
		err = "Not enough arguments";
		return false;
	}

	bool hasWidth  = false;
	bool hasHeight = false;
	string lastCmd = "";

	for (int i = 1; i < argc; i++)
	{
		string currCmd = argv[i];
		if (currCmd[0] == '-' && lastCmd.empty())
		{
			auto search = argsMap.find(currCmd);
			if (search == argsMap.end())
			{
				err = "Unknown argument: " + currCmd;
				return false;
			}

			if (!search->second.hasValue)
				config.flags |= flagsMap[search->second.argument];
			else
				lastCmd = search->first;

			continue;
		}

		CMDParameter cmd = argsMap[lastCmd];
		switch (cmd.argument)
		{
			case WIDTH:
			{
				int width = ParseInt(currCmd);
				if (width <= 0)
				{
					err = "Invalud value passed to: " + lastCmd;
					return false;
				}
				config.screenWidth = width;
				hasWidth = true;
			}
			break;

			case HEIGHT:
			{
				int height = ParseInt(currCmd);
				if (height <= 0)
				{
					err = "Invalid value passed to: " + lastCmd;
					return false;
				}
				config.screenHeight = height;
				hasHeight = true;
			}
			break;
		}
		lastCmd = "";
	}
	if (!hasWidth || !hasHeight)
	{
		err = "Window dimensions missing";
		return false;
	}
	return true;
}
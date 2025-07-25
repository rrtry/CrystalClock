#include "raylib.h"
#include "config.h"

#include <map>
#include <fstream>
#include <algorithm>

using namespace std;

static map<Argument, int> windowFlagsMap = {
	{ BORDERLESS,  FLAG_BORDERLESS_WINDOWED_MODE },
	{ FULLSCREEN,  FLAG_FULLSCREEN_MODE			 },
	{ UNDECORATED, FLAG_WINDOW_UNDECORATED		 }
};

static map<Argument, int> prefFlagsMap = {
	{ NO_SOUND,   FLAG_NO_SOUND   },
	{ NO_FADE_IN, FLAG_NO_FADE_IN }
};

static map<string, CMDParameter> argsMap = {

	{ CMD_WIDTH,  { WIDTH,  true }},
	{ CMD_HEIGHT, { HEIGHT, true }},

	{ CMD_WIDTH_SHORT,  { WIDTH,  true }},
	{ CMD_HEIGHT_SHORT, { HEIGHT, true }},

	{ CMD_DISPLAY, 		 { DISPLAY, true }},
	{ CMD_DISPLAY_SHORT, { DISPLAY, true }},

	{ CMD_BORDERLESS,  { BORDERLESS,  false }},
	{ CMD_FULLSCREEN,  { FULLSCREEN,  false }},
	{ CMD_UNDECORATED, { UNDECORATED, false }},

	{ CMD_NO_FADE_IN, { NO_FADE_IN, false }},
	{ CMD_NO_SOUND,   { NO_SOUND,   false }}
};

static inline void ltrim(string& s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
        return !std::isspace(ch);
    }));
}

static inline void rtrim(string& s) {
    s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) {
        return !std::isspace(ch);
    }).base(), s.end());
}

static bool ParseInt(const string& cmd, int& value)
{
	try 
	{
		value = stoi(cmd);
		return true;
	}
	catch (exception& err)
	{
		return false;
	}
}

static bool ParseArgValue(Config& config,
						  const CMDParameter& cmd, 
						  const string& arg, 
						  const string& argValue)
{
	int ivalue  = -1;
	bool parsed = false;

	switch (cmd.argument)
	{
		case WIDTH:
		{
			parsed = ParseInt(argValue, ivalue) && ivalue > 0;
			if (parsed)
				config.screenWidth = ivalue;
		}
		break;

		case HEIGHT:
		{
			parsed = ParseInt(argValue, ivalue) && ivalue > 0;
			if (parsed)
				config.screenHeight = ivalue;
		}
		break;

		case DISPLAY:
		{
			parsed = ParseInt(argValue, ivalue) && ivalue > -1;
			if (parsed)
				config.display = ivalue;
		}
		break;

		case NO_SOUND:
		case NO_FADE_IN:
		{
			parsed = ParseInt(argValue, ivalue) && ivalue == 1;
			if (parsed)
				config.preferenceFlags |= prefFlagsMap[cmd.argument];
		}
		break;

		case FULLSCREEN:
		case BORDERLESS:
		case UNDECORATED:
		{
			parsed = ParseInt(argValue, ivalue) && ivalue == 1;
			if (parsed)
				config.flags |= windowFlagsMap[cmd.argument];
		}
		break;
	}
	return parsed;
}

bool ParseCMD(Config& config, int argc, char** argv, string& err, bool prefsOnly)
{
	config = { 0 };
	if (!prefsOnly && argc < 5)
	{
		err = "Not enough arguments";
		return false;
	}
	
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
			{
				if (prefFlagsMap.find(search->second.argument) != prefFlagsMap.end())
					config.preferenceFlags |= prefFlagsMap[search->second.argument];
				else
					config.flags |= windowFlagsMap[search->second.argument];
			}
			else
			{
				lastCmd = search->first;
			}
			continue;
		}

		CMDParameter cmd = argsMap[lastCmd];
		if (!ParseArgValue(config, cmd, lastCmd, currCmd))
		{
			err = "Invalid value passed to " + lastCmd;
			return false;
		}
		lastCmd = "";
	}
	if (!prefsOnly)
	{
		if (config.screenWidth <= 0 || config.screenHeight <= 0)
		{
			err = "Window dimensions missing";
			return false;
		}
	}
	return true;
}

/*
* Simple INI-like parser, supports only simple key-value pairs, without special characters and line continuation.
* Sections are ignored for now.
*/
bool ParseINI(Config& cfg, const string& path, bool prefsOnly)
{
	cfg = { 0 };
    ifstream ifs(path);

    if (!ifs.is_open())
        return false;

    string line;
    string key;
    string value;

    while (getline(ifs, line))
    {
        if (line.empty())
            continue;

        char ch = line.at(0);
        if (ch == '[' || ch == ';' || ch == '#')
            continue;

        const auto delim = line.find_first_of('=', 0);
        if (delim == string::npos)
            continue;

        if (delim == line.length() - 1)
            continue;

        auto from  = delim + 1;
        auto count = line.length() - from;

        key   = line.substr(0, delim);
        value = line.substr(from, count);

        if (key.empty() || value.empty())
            continue;

        ltrim(key);
        ltrim(value);

        rtrim(key);
        rtrim(value);

        if (value.at(0) == '"' && value.at(value.length() - 1) == '"')
        {
            value.erase(0, 1);
            value.erase(value.length() - 1, 1);
        }

		key = '-' + key;
		auto search = argsMap.find(key);
		if (search != argsMap.end())
		{
			ParseArgValue(cfg, search->second, key, value);	
		}
	}
	return (prefsOnly || (cfg.screenWidth > 0 && cfg.screenHeight > 0));
}

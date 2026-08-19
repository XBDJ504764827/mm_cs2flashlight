#include "gamedata.h"

#include <cctype>
#include <cstdlib>
#include <fstream>

#include <tier1/strtools.h>

namespace
{
	std::string Trim(const std::string &value)
	{
		size_t first = 0;
		while (first < value.size() && std::isspace(static_cast<unsigned char>(value[first])))
		{
			++first;
		}

		size_t last = value.size();
		while (last > first && std::isspace(static_cast<unsigned char>(value[last - 1])))
		{
			--last;
		}

		return value.substr(first, last - first);
	}

	void SetError(char *error, size_t maxlen, const char *format, const char *value)
	{
		if (error != nullptr && maxlen > 0)
		{
			V_snprintf(error, maxlen, format, value);
		}
	}
}

bool FlashlightGameData::Load(const char *path, char *error, size_t maxlen)
{
	std::ifstream input(path);
	if (!input.is_open())
	{
		SetError(error, maxlen, "Unable to open gamedata file: %s", path);
		return false;
	}

#if defined(_WIN32)
	const std::string currentPlatform = "windows";
#else
	const std::string currentPlatform = "linux";
#endif

	std::string section;
	std::string line;
	while (std::getline(input, line))
	{
		const size_t comment = line.find_first_of("#;");
		if (comment != std::string::npos)
		{
			line.erase(comment);
		}

		line = Trim(line);
		if (line.empty())
		{
			continue;
		}

		if (line.front() == '[' && line.back() == ']')
		{
			section = Trim(line.substr(1, line.size() - 2));
			continue;
		}

		if (section != currentPlatform)
		{
			continue;
		}

		const size_t separator = line.find('=');
		if (separator == std::string::npos)
		{
			SetError(error, maxlen, "Invalid gamedata line: %s", line.c_str());
			return false;
		}

		const std::string key = Trim(line.substr(0, separator));
		const std::string value = Trim(line.substr(separator + 1));

		if (key == "game_entity_system_offset" || key == "teleport_virtual_index")
		{
			char *end = nullptr;
			const long parsed = std::strtol(value.c_str(), &end, 10);
			if (end == value.c_str() || *end != '\0' || parsed < 0)
			{
				SetError(error, maxlen, "Invalid numeric gamedata value: %s", value.c_str());
				return false;
			}

			if (key == "game_entity_system_offset")
			{
				gameEntitySystemOffset = static_cast<int>(parsed);
			}
			else
			{
				teleportVirtualIndex = static_cast<int>(parsed);
			}
		}
		else if (key == "create_entity_by_name")
		{
			createEntityByName = value;
		}
		else if (key == "dispatch_spawn")
		{
			dispatchSpawn = value;
		}
		else if (key == "accept_input")
		{
			acceptInput = value;
		}
	}

	if (gameEntitySystemOffset < 0 || teleportVirtualIndex < 0 || createEntityByName.empty() ||
			dispatchSpawn.empty() || acceptInput.empty())
	{
		SetError(error, maxlen, "Gamedata is incomplete for platform: %s", currentPlatform.c_str());
		return false;
	}

	return true;
}

#ifndef CS2_FLASHLIGHT_GAMEDATA_H
#define CS2_FLASHLIGHT_GAMEDATA_H

#include <cstddef>
#include <string>

struct FlashlightGameData
{
	int gameEntitySystemOffset = -1;
	int teleportVirtualIndex = -1;
	std::string createEntityByName;
	std::string dispatchSpawn;
	std::string acceptInput;

	bool Load(const char *path, char *error, size_t maxlen);
};

#endif // CS2_FLASHLIGHT_GAMEDATA_H

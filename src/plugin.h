/**
 * CS2 Flashlight Metamod:Source plugin.
 */

#ifndef CS2_FLASHLIGHT_PLUGIN_H
#define CS2_FLASHLIGHT_PLUGIN_H

#include <array>

#include <ISmmPlugin.h>
#include <eiface.h>
#include <entityhandle.h>

#include "version_gen.h"

class CEntityInstance;

class MMSPlugin final : public ISmmPlugin, public IMetamodListener
{
public:
	bool Load(PluginId id, ISmmAPI *ismm, char *error, size_t maxlen, bool late) override;
	bool Unload(char *error, size_t maxlen) override;

	void OnLevelInit(char const *pMapName, char const *pMapEntities,
			char const *pOldLevel, char const *pLandmarkName, bool loadGame,
			bool background) override;
	void OnLevelShutdown() override;

	const char *GetAuthor() override { return PLUGIN_AUTHOR; }
	const char *GetName() override { return PLUGIN_DISPLAY_NAME; }
	const char *GetDescription() override { return PLUGIN_DESCRIPTION; }
	const char *GetURL() override { return PLUGIN_URL; }
	const char *GetLicense() override { return PLUGIN_LICENSE; }
	const char *GetVersion() override { return PLUGIN_FULL_VERSION; }
	const char *GetDate() override { return __DATE__; }
	const char *GetLogTag() override { return PLUGIN_LOGTAG; }

private:
	static constexpr int kMaxPlayerSlots = 64;

	void OnGameFrame(bool simulating, bool firstTick, bool lastTick);
	void OnClientDisconnect(CPlayerSlot slot, ENetworkDisconnectionReason reason,
			const char *name, uint64 xuid, const char *networkId);
	void UpdateEntitySystem();
	CEntityInstance *CreatePlayerLight(CEntityInstance *pawn);
	void TogglePlayerLight(int slot, CEntityInstance *pawn,
			const CEntityHandle &pawnHandle);
	void RemovePlayerLight(int slot);
	void RemoveAllLights();
	void ClearPlayerState();

	std::array<CEntityHandle, kMaxPlayerSlots> m_playerLights;
	std::array<CEntityHandle, kMaxPlayerSlots> m_playerPawns;
	std::array<bool, kMaxPlayerSlots> m_lightEnabled{};
	bool m_gameFrameHooked = false;
	bool m_clientDisconnectHooked = false;
};

extern MMSPlugin g_ThisPlugin;

PLUGIN_GLOBALVARS();

#endif // CS2_FLASHLIGHT_PLUGIN_H

/**
 * CS2 Flashlight Metamod:Source plugin.
 */

#ifndef CS2_FLASHLIGHT_PLUGIN_H
#define CS2_FLASHLIGHT_PLUGIN_H

#include <ISmmPlugin.h>
#include <eiface.h>
#include <tier1/convar.h>

#include "version_gen.h"

class MMSPlugin final : public ISmmPlugin, public IMetamodListener
{
public:
	bool Load(PluginId id, ISmmAPI *ismm, char *error, size_t maxlen, bool late) override;
	bool Unload(char *error, size_t maxlen) override;

	const char *GetAuthor() override { return PLUGIN_AUTHOR; }
	const char *GetName() override { return PLUGIN_DISPLAY_NAME; }
	const char *GetDescription() override { return PLUGIN_DESCRIPTION; }
	const char *GetURL() override { return PLUGIN_URL; }
	const char *GetLicense() override { return PLUGIN_LICENSE; }
	const char *GetVersion() override { return PLUGIN_FULL_VERSION; }
	const char *GetDate() override { return __DATE__; }
	const char *GetLogTag() override { return PLUGIN_LOGTAG; }

private:
	void OnClientCommand(CPlayerSlot slot, const CCommand &args);
	bool IsFlashlightCommand(const CCommand &args) const;
	void ForwardFlashlightCommand(CPlayerSlot slot);

	bool m_clientCommandHooked = false;
};

extern MMSPlugin g_ThisPlugin;

PLUGIN_GLOBALVARS();

#endif // CS2_FLASHLIGHT_PLUGIN_H

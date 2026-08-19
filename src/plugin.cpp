/**
 * CS2 Flashlight Metamod:Source plugin.
 *
 * CS2 clients send the legacy "impulse 100" command for the flashlight
 * action. The game client is responsible for rendering the light, so the
 * plugin forwards the command back to the originating client. This keeps the
 * implementation independent of private entity layouts and schema offsets.
 */

#include "plugin.h"

namespace
{
	IServerGameClients *g_gameClients = nullptr;
	IVEngineServer *g_engineServer = nullptr;

	bool IsCommand(const CCommand &args, const char *name)
	{
		return args.ArgC() > 0 && V_stricmp(args.Arg(0), name) == 0;
	}
}

SH_DECL_HOOK2_void(IServerGameClients, ClientCommand, SH_NOATTRIB, 0,
		CPlayerSlot, const CCommand &);

MMSPlugin g_ThisPlugin;
PLUGIN_EXPOSE(MMSPlugin, g_ThisPlugin);

bool MMSPlugin::Load(PluginId id, ISmmAPI *ismm, char *error, size_t maxlen, bool late)
{
	PLUGIN_SAVEVARS();

	GET_V_IFACE_ANY(GetEngineFactory, g_engineServer, IVEngineServer,
			INTERFACEVERSION_VENGINESERVER);
	GET_V_IFACE_ANY(GetServerFactory, g_gameClients, IServerGameClients,
			INTERFACEVERSION_SERVERGAMECLIENTS);

	if (g_engineServer == nullptr || g_gameClients == nullptr)
	{
		if (error != nullptr && maxlen > 0)
		{
			V_snprintf(error, maxlen, "Required Source2 game interfaces are unavailable");
		}
		return false;
	}

	SH_ADD_HOOK(IServerGameClients, ClientCommand, g_gameClients,
			SH_MEMBER(this, &MMSPlugin::OnClientCommand), false);
	m_clientCommandHooked = true;

	return true;
}

bool MMSPlugin::Unload(char *error, size_t maxlen)
{
	if (m_clientCommandHooked)
	{
		SH_REMOVE_HOOK(IServerGameClients, ClientCommand, g_gameClients,
				SH_MEMBER(this, &MMSPlugin::OnClientCommand), false);
		m_clientCommandHooked = false;
	}

	return true;
}

bool MMSPlugin::IsFlashlightCommand(const CCommand &args) const
{
	if (args.ArgC() == 0)
	{
		return false;
	}

	// impulse 100 is the engine's flashlight toggle. Some CS2 configs use
	// +lookatweapon for the F key, so accept that press as a compatibility path.
	if (IsCommand(args, "impulse"))
	{
		return args.ArgC() > 1 && V_stricmp(args.Arg(1), "100") == 0;
	}

	return IsCommand(args, "+lookatweapon");
}

void MMSPlugin::ForwardFlashlightCommand(CPlayerSlot slot)
{
	if (g_engineServer == nullptr)
	{
		return;
	}

	// ClientCommand executes in the client's console and does not re-enter the
	// server-side ClientCommand hook. Therefore a consumed input cannot recurse.
	g_engineServer->ClientCommand(slot, "impulse 100");
}

void MMSPlugin::OnClientCommand(CPlayerSlot slot, const CCommand &args)
{
	if (!IsFlashlightCommand(args))
	{
		RETURN_META(MRES_IGNORED);
	}

	ForwardFlashlightCommand(slot);
	RETURN_META(MRES_SUPERCEDE);
}

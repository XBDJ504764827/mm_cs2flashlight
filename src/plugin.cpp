/**
 * CS2 Flashlight Metamod:Source plugin.
 */

#include "plugin.h"

#include "gamedata.h"
#include "module_scanner.h"
#include "schema.h"

#include <algorithm>
#include <cstdint>

#include <Color.h>
#include <entity2/entityinstance.h>
#include <entity2/entitykeyvalues.h>
#include <entity2/entitysystem.h>
#include <in_buttons.h>
#include <interfaces/interfaces.h>
#include <mathlib/mathlib.h>
#include <schemasystem/schemasystem.h>
#include <tier1/strtools.h>
#include <variant.h>

namespace
{
	constexpr float kFlashlightBrightness = 3.0f;
	constexpr float kFlashlightForwardOffset = 54.0f;
	constexpr float kPlayerEyeHeight = 64.0f;

	IServerGameDLL *g_server = nullptr;
	IServerGameClients *g_gameClients = nullptr;
	IVEngineServer *g_engineServer = nullptr;
	IGameResourceService *g_gameResourceService = nullptr;
	ISchemaSystem *g_schemaSystem = nullptr;
	CGameEntitySystem *g_entitySystem = nullptr;

	using CreateEntityByNameFn = CEntityInstance *(*)(const char *, int);
	using DispatchSpawnFn = void (*)(CEntityInstance *, CEntityKeyValues *);
	using AcceptInputFn = void (*)(CEntityInstance *, const char *, CEntityInstance *,
			CEntityInstance *, variant_t *);
	using TeleportFn = void (*)(CEntityInstance *, const Vector *, const QAngle *,
			const Vector *);

	CreateEntityByNameFn g_createEntityByName = nullptr;
	DispatchSpawnFn g_dispatchSpawn = nullptr;
	AcceptInputFn g_acceptInput = nullptr;

	FlashlightGameData g_gameData;
	SchemaResolver g_schema;

	struct SchemaOffsets
	{
		int controllerPawn = -1;
		int controllerPawnAlive = -1;
		int pawnMovementServices = -1;
		int movementButtons = -1;
		int buttonStates = -1;
		int pawnEyeAngles = -1;
		int entityBodyComponent = -1;
		int bodySceneNode = -1;
		int sceneAbsOrigin = -1;
		int lightEnabled = -1;
		int lightColor = -1;
		int lightBrightness = -1;
		int lightRange = -1;
		int lightSoftX = -1;
		int lightSoftY = -1;
		int lightSkirt = -1;
		int lightSkirtNear = -1;
		int lightSizeParams = -1;
		int lightCastShadows = -1;
		int lightDirectLight = -1;
	} g_offsets;

	template <typename T>
	T &Field(void *object, int offset)
	{
		return *reinterpret_cast<T *>(reinterpret_cast<uint8_t *>(object) + offset);
	}

	void SetError(char *error, size_t maxlen, const char *format, const char *value)
	{
		if (error != nullptr && maxlen > 0)
		{
			V_snprintf(error, maxlen, format, value);
		}
	}

	bool ResolveOffset(int &destination, const char *className, const char *fieldName,
			char *error, size_t maxlen)
	{
		destination = g_schema.FindOffset(className, fieldName);
		if (destination >= 0)
		{
			return true;
		}

		char field[256];
		V_snprintf(field, sizeof(field), "%s::%s", className, fieldName);
		SetError(error, maxlen, "Unable to resolve schema field: %s", field);
		return false;
	}

	bool ResolveSchemaOffsets(char *error, size_t maxlen)
	{
		return ResolveOffset(g_offsets.controllerPawn, "CBasePlayerController", "m_hPawn", error, maxlen) &&
			ResolveOffset(g_offsets.controllerPawnAlive, "CCSPlayerController", "m_bPawnIsAlive", error, maxlen) &&
			ResolveOffset(g_offsets.pawnMovementServices, "CBasePlayerPawn", "m_pMovementServices", error, maxlen) &&
			ResolveOffset(g_offsets.movementButtons, "CPlayer_MovementServices", "m_nButtons", error, maxlen) &&
			ResolveOffset(g_offsets.buttonStates, "CInButtonState", "m_pButtonStates", error, maxlen) &&
			ResolveOffset(g_offsets.pawnEyeAngles, "CCSPlayerPawn", "m_angEyeAngles", error, maxlen) &&
			ResolveOffset(g_offsets.entityBodyComponent, "CBaseEntity", "m_CBodyComponent", error, maxlen) &&
			ResolveOffset(g_offsets.bodySceneNode, "CBodyComponent", "m_pSceneNode", error, maxlen) &&
			ResolveOffset(g_offsets.sceneAbsOrigin, "CGameSceneNode", "m_vecAbsOrigin", error, maxlen) &&
			ResolveOffset(g_offsets.lightEnabled, "CBarnLight", "m_bEnabled", error, maxlen) &&
			ResolveOffset(g_offsets.lightColor, "CBarnLight", "m_Color", error, maxlen) &&
			ResolveOffset(g_offsets.lightBrightness, "CBarnLight", "m_flBrightness", error, maxlen) &&
			ResolveOffset(g_offsets.lightRange, "CBarnLight", "m_flRange", error, maxlen) &&
			ResolveOffset(g_offsets.lightSoftX, "CBarnLight", "m_flSoftX", error, maxlen) &&
			ResolveOffset(g_offsets.lightSoftY, "CBarnLight", "m_flSoftY", error, maxlen) &&
			ResolveOffset(g_offsets.lightSkirt, "CBarnLight", "m_flSkirt", error, maxlen) &&
			ResolveOffset(g_offsets.lightSkirtNear, "CBarnLight", "m_flSkirtNear", error, maxlen) &&
			ResolveOffset(g_offsets.lightSizeParams, "CBarnLight", "m_vSizeParams", error, maxlen) &&
			ResolveOffset(g_offsets.lightCastShadows, "CBarnLight", "m_nCastShadows", error, maxlen) &&
			ResolveOffset(g_offsets.lightDirectLight, "CBarnLight", "m_nDirectLight", error, maxlen);
	}

	void AcceptInput(CEntityInstance *entity, const char *input, const char *parameter = "",
			CEntityInstance *activator = nullptr)
	{
		variant_t value(parameter);
		g_acceptInput(entity, input, activator, nullptr, &value);
	}

	void Teleport(CEntityInstance *entity, const Vector *position, const QAngle *angles)
	{
		void **virtualTable = *reinterpret_cast<void ***>(entity);
		auto teleport = reinterpret_cast<TeleportFn>(
				virtualTable[g_gameData.teleportVirtualIndex]);
		teleport(entity, position, angles, nullptr);
	}
}

CGameEntitySystem *GameEntitySystem()
{
	return g_entitySystem;
}

SH_DECL_HOOK3_void(IServerGameDLL, GameFrame, SH_NOATTRIB, 0, bool, bool, bool);
SH_DECL_HOOK5_void(IServerGameClients, ClientDisconnect, SH_NOATTRIB, 0,
		CPlayerSlot, ENetworkDisconnectionReason, const char *, uint64, const char *);

MMSPlugin g_ThisPlugin;
PLUGIN_EXPOSE(MMSPlugin, g_ThisPlugin);

bool MMSPlugin::Load(PluginId id, ISmmAPI *ismm, char *error, size_t maxlen, bool late)
{
	PLUGIN_SAVEVARS();

	GET_V_IFACE_ANY(GetServerFactory, g_server, IServerGameDLL,
			INTERFACEVERSION_SERVERGAMEDLL);
	GET_V_IFACE_ANY(GetServerFactory, g_gameClients, IServerGameClients,
			INTERFACEVERSION_SERVERGAMECLIENTS);
	GET_V_IFACE_ANY(GetEngineFactory, g_engineServer, IVEngineServer,
			INTERFACEVERSION_VENGINESERVER);
	GET_V_IFACE_CURRENT(GetEngineFactory, g_gameResourceService, IGameResourceService,
			GAMERESOURCESERVICESERVER_INTERFACE_VERSION);
	GET_V_IFACE_CURRENT(GetEngineFactory, g_schemaSystem, ISchemaSystem,
			SCHEMASYSTEM_INTERFACE_VERSION);

	char gamedataPath[1024];
	g_SMAPI->PathFormat(gamedataPath, sizeof(gamedataPath),
			"%s/addons/cs2_flashlight/gamedata/cs2_flashlight.games.txt",
			g_SMAPI->GetBaseDir());
	if (!g_gameData.Load(gamedataPath, error, maxlen))
	{
		return false;
	}

	ModuleScanner scanner;
	void *serverAddress = (*reinterpret_cast<void ***>(g_server))[0];
	if (!scanner.Initialize(serverAddress))
	{
		SetError(error, maxlen, "Unable to inspect server module: %s", "module not found");
		return false;
	}

	g_createEntityByName = reinterpret_cast<CreateEntityByNameFn>(
			scanner.FindPattern(g_gameData.createEntityByName));
	g_dispatchSpawn = reinterpret_cast<DispatchSpawnFn>(
			scanner.FindPattern(g_gameData.dispatchSpawn));
	g_acceptInput = reinterpret_cast<AcceptInputFn>(
			scanner.FindPattern(g_gameData.acceptInput));
	if (g_createEntityByName == nullptr || g_dispatchSpawn == nullptr || g_acceptInput == nullptr)
	{
		SetError(error, maxlen, "Unable to resolve engine function: %s",
			"check cs2_flashlight.games.txt after the latest CS2 update");
		return false;
	}

	g_schema.SetSchemaSystem(g_schemaSystem);
	if (!ResolveSchemaOffsets(error, maxlen))
	{
		return false;
	}

	g_SMAPI->AddListener(this, this);
	SH_ADD_HOOK(IServerGameDLL, GameFrame, g_server,
			SH_MEMBER(this, &MMSPlugin::OnGameFrame), false);
	m_gameFrameHooked = true;
	SH_ADD_HOOK(IServerGameClients, ClientDisconnect, g_gameClients,
			SH_MEMBER(this, &MMSPlugin::OnClientDisconnect), true);
	m_clientDisconnectHooked = true;

	META_CONPRINTF("[%s] Loaded version %s; F uses IN_LOOK_AT_WEAPON.\n",
			PLUGIN_LOGTAG, PLUGIN_FULL_VERSION);
	return true;
}

bool MMSPlugin::Unload(char *error, size_t maxlen)
{
	if (m_gameFrameHooked)
	{
		SH_REMOVE_HOOK(IServerGameDLL, GameFrame, g_server,
				SH_MEMBER(this, &MMSPlugin::OnGameFrame), false);
		m_gameFrameHooked = false;
	}

	if (m_clientDisconnectHooked)
	{
		SH_REMOVE_HOOK(IServerGameClients, ClientDisconnect, g_gameClients,
				SH_MEMBER(this, &MMSPlugin::OnClientDisconnect), true);
		m_clientDisconnectHooked = false;
	}

	RemoveAllLights();
	return true;
}

void MMSPlugin::OnLevelInit(char const *pMapName, char const *pMapEntities,
		char const *pOldLevel, char const *pLandmarkName, bool loadGame, bool background)
{
	ClearPlayerState();
}

void MMSPlugin::OnLevelShutdown()
{
	ClearPlayerState();
	g_entitySystem = nullptr;
}

void MMSPlugin::UpdateEntitySystem()
{
	if (g_gameResourceService == nullptr)
	{
		g_entitySystem = nullptr;
		return;
	}

	g_entitySystem = *reinterpret_cast<CGameEntitySystem **>(
			reinterpret_cast<uint8_t *>(g_gameResourceService) +
			g_gameData.gameEntitySystemOffset);
}

void MMSPlugin::OnGameFrame(bool simulating, bool firstTick, bool lastTick)
{
	if (!simulating)
	{
		RETURN_META(MRES_IGNORED);
	}

	UpdateEntitySystem();
	if (g_entitySystem == nullptr || g_engineServer == nullptr ||
			g_engineServer->GetServerGlobals() == nullptr)
	{
		RETURN_META(MRES_IGNORED);
	}

	const int maxClients = std::min(g_engineServer->GetServerGlobals()->maxClients,
			kMaxPlayerSlots);
	for (int slot = 0; slot < maxClients; ++slot)
	{
		CEntityInstance *controller = g_entitySystem->GetEntityInstance(CEntityIndex(slot + 1));
		if (controller == nullptr || !Field<bool>(controller, g_offsets.controllerPawnAlive))
		{
			RemovePlayerLight(slot);
			continue;
		}

		const CEntityHandle pawnHandle = Field<CEntityHandle>(controller, g_offsets.controllerPawn);
		CEntityInstance *pawn = g_entitySystem->GetEntityInstance(pawnHandle);
		if (pawn == nullptr)
		{
			RemovePlayerLight(slot);
			continue;
		}

		if (m_playerPawns[slot].IsValid() && m_playerPawns[slot] != pawnHandle)
		{
			RemovePlayerLight(slot);
		}

		void *movementServices = Field<void *>(pawn, g_offsets.pawnMovementServices);
		if (movementServices == nullptr)
		{
			continue;
		}

		void *buttonState = reinterpret_cast<uint8_t *>(movementServices) +
			g_offsets.movementButtons;
		const uint64 *buttons = reinterpret_cast<const uint64 *>(
			reinterpret_cast<uint8_t *>(buttonState) + g_offsets.buttonStates);
		if ((buttons[0] & IN_LOOK_AT_WEAPON) != 0 &&
				(buttons[1] & IN_LOOK_AT_WEAPON) != 0)
		{
			TogglePlayerLight(slot, pawn, pawnHandle);
		}

		CEntityInstance *light = g_entitySystem->GetEntityInstance(m_playerLights[slot]);
		if (light != nullptr && m_lightEnabled[slot])
		{
			UpdatePlayerLightTransform(pawn, light);
		}
	}

	RETURN_META(MRES_IGNORED);
}

void MMSPlugin::OnClientDisconnect(CPlayerSlot slot, ENetworkDisconnectionReason reason,
		const char *name, uint64 xuid, const char *networkId)
{
	const int index = slot.Get();
	if (index >= 0 && index < kMaxPlayerSlots)
	{
		RemovePlayerLight(index);
	}

	RETURN_META(MRES_IGNORED);
}

CEntityInstance *MMSPlugin::CreatePlayerLight(CEntityInstance *pawn)
{
	void *bodyComponent = Field<void *>(pawn, g_offsets.entityBodyComponent);
	if (bodyComponent == nullptr)
	{
		return nullptr;
	}

	void *sceneNode = Field<void *>(bodyComponent, g_offsets.bodySceneNode);
	if (sceneNode == nullptr)
	{
		return nullptr;
	}

	CEntityInstance *light = g_createEntityByName("light_barn", -1);
	if (light == nullptr)
	{
		return nullptr;
	}

	Field<bool>(light, g_offsets.lightEnabled) = false;
	Field<Color>(light, g_offsets.lightColor).SetColor(255, 255, 255, 255);
	Field<float>(light, g_offsets.lightBrightness) = kFlashlightBrightness;
	Field<float>(light, g_offsets.lightRange) = 2048.0f;
	Field<float>(light, g_offsets.lightSoftX) = 1.0f;
	Field<float>(light, g_offsets.lightSoftY) = 1.0f;
	Field<float>(light, g_offsets.lightSkirt) = 0.5f;
	Field<float>(light, g_offsets.lightSkirtNear) = 1.0f;
	Field<Vector>(light, g_offsets.lightSizeParams).Init(45.0f, 45.0f, 0.02f);
	Field<int>(light, g_offsets.lightCastShadows) = 1;
	Field<int>(light, g_offsets.lightDirectLight) = 3;

	auto *keyValues = new CEntityKeyValues();
	keyValues->SetString("lightcookie", "materials/effects/lightcookies/flashlight.vtex");
	g_dispatchSpawn(light, keyValues);

	UpdatePlayerLightTransform(pawn, light);
	light->NetworkStateChanged(NetworkStateChangedData(true));
	return light;
}

void MMSPlugin::UpdatePlayerLightTransform(CEntityInstance *pawn, CEntityInstance *light)
{
	void *bodyComponent = Field<void *>(pawn, g_offsets.entityBodyComponent);
	if (bodyComponent == nullptr)
	{
		return;
	}

	void *sceneNode = Field<void *>(bodyComponent, g_offsets.bodySceneNode);
	if (sceneNode == nullptr)
	{
		return;
	}

	const QAngle eyeAngles = Field<QAngle>(pawn, g_offsets.pawnEyeAngles);
	Vector origin = Field<Vector>(sceneNode, g_offsets.sceneAbsOrigin);
	Vector forward;
	AngleVectors(eyeAngles, &forward);
	origin.z += kPlayerEyeHeight;
	origin += forward * kFlashlightForwardOffset;

	Teleport(light, &origin, &eyeAngles);
}

void MMSPlugin::TogglePlayerLight(int slot, CEntityInstance *pawn,
		const CEntityHandle &pawnHandle)
{
	CEntityInstance *light = g_entitySystem->GetEntityInstance(m_playerLights[slot]);
	if (light == nullptr)
	{
		m_playerLights[slot].Term();
		m_lightEnabled[slot] = false;
		light = CreatePlayerLight(pawn);
		if (light == nullptr)
		{
			META_CONPRINTF("[%s] Failed to create light_barn for player slot %d.\n",
					PLUGIN_LOGTAG, slot);
			return;
		}

		m_playerLights[slot] = light->GetRefEHandle();
		m_playerPawns[slot] = pawnHandle;
	}

	AcceptInput(light, m_lightEnabled[slot] ? "Disable" : "Enable");
	m_lightEnabled[slot] = !m_lightEnabled[slot];
}

void MMSPlugin::RemovePlayerLight(int slot)
{
	if (slot < 0 || slot >= kMaxPlayerSlots)
	{
		return;
	}

	if (g_entitySystem != nullptr && g_acceptInput != nullptr)
	{
		CEntityInstance *light = g_entitySystem->GetEntityInstance(m_playerLights[slot]);
		if (light != nullptr)
		{
			AcceptInput(light, "Kill");
		}
	}

	m_playerLights[slot].Term();
	m_playerPawns[slot].Term();
	m_lightEnabled[slot] = false;
}

void MMSPlugin::RemoveAllLights()
{
	UpdateEntitySystem();
	for (int slot = 0; slot < kMaxPlayerSlots; ++slot)
	{
		RemovePlayerLight(slot);
	}
}

void MMSPlugin::ClearPlayerState()
{
	for (int slot = 0; slot < kMaxPlayerSlots; ++slot)
	{
		m_playerLights[slot].Term();
		m_playerPawns[slot].Term();
		m_lightEnabled[slot] = false;
	}
}

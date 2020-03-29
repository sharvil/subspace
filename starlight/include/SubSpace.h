#pragma once

#include "base.h"

typedef uint16 plid_t;
static const plid_t KInvalidPlid = static_cast <plid_t> (-1);
static const uint32 KMaximumChatSize = 256;

typedef uint16 frequency_t;

enum EShipType
{
	KShipWarbird,
	KShipJavelin,
	KShipSpider,
	KShipLeviathan,
	KShipTerrier,
	KShipWeasel,
	KShipLancaster,
	KShipShark,
	KShipSpectator
};

enum EChatType
{
	KChatArena,
	KChatPublicMacro,
	KChatPublic,
	KChatFrequency,
	KChatRemoteFrequency,
	KChatPrivate,
	KChatWarning,
	KChatRemotePrivate,
	KChatError,
	KChatChannel
};

enum ELoginResponse
{
	KLoginOk,
	KLoginUnknownUser,
	KLoginInvalidPassword,
	KLoginZoneFull,
	KLoginLockedOut,
	KLoginNoPermission,
	KLoginSpectateOnly,
	KLoginTooManyPoints,
	KLoginSlowConnection,
	KLoginNoPermission2,
	KLoginNoNewConnections,
	KLoginInvalidName,
	KLoginObsceneName,
	KLoginBillerDown,
	KLoginBusyProcessing,
	KLoginExperiencedOnly,
	KLoginAskDemographics,
	KLoginTooManyDemos,
	KLoginClosedToDemos,
	KLoginUnknown,
	KLoginNeedModerator = 255
};

struct Player
{
	plid_t iId;
	std::string iName;
	std::string iSquad;

	EShipType iShip;
	frequency_t iFrequency;

	uint32 iFlagPoints;
	uint32 iKillPoints;
	uint32 iWins;
	uint32 iLosses;
	uint32 iFlagsCarried;
};

struct ArenaEntry
{
	std::string iName;
	std::string iDisplayName;
	int32 iPopulation;
	bool iInArena;
};

typedef std::list <ArenaEntry> ArenaList;

/*++

Copyright (c) 2010 Sharvil Nanavati.

Module:

    Zone.h

Authors:

    Sharvil Nanavati (sharvil) 2010-05-06

--*/

#pragma once

#include "base.h"
#include "NetLib.h"

class Logger;
class ZonePacket;
class ZoneUpdatePacket;

struct Zone
{
	Zone();

	void OnListing(const ZonePacket & packet);
	void OnUpdate(const ZoneUpdatePacket & packet);

	bool IsDisabled() const;      // Disabled zones should not show up in zone listings.
	bool IsStale() const;         // Zones should be purged if they are stale.

	bool iIsMaster;

	uint32 iUpdateTime;

	InetAddress iAddress;
	string iName;
	string iPassword;
	string iDescription;

	uint32 iServerVersion;
	uint16 iPlayerCount;
	bool iIsBilled;
	uint8 iHopCount;

	uint16 iMinPlayerCount;
	uint16 iMaxPlayerCount;

	private:
		Logger & iLogger;
};

typedef std::list <Zone> Zones;

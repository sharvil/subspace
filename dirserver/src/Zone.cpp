/*++

Copyright (c) 2010 Sharvil Nanavati.

Module:

    Zone.cpp

Authors:

    Sharvil Nanavati (sharvil) 2010-05-18

--*/

#include "base.h"
#include "Zone.h"
#include "LogLib.h"
#include "TimeLib.h"

#include "dirserver.proto.h"

Zone::Zone()
	: iIsMaster(false),
	  iHopCount(~0),
	  iLogger(Logger::Instance())
{
}

//
// A peer directory server has provided a listing for this zone.
//
void Zone::OnListing(const ZonePacket & packet)
{
	if(iIsMaster || iHopCount < packet.iHopCount)
		return;

	iUpdateTime = Time::GetMilliCount();
	iAddress = InetAddress(Primitive::Htonl(packet.iIp), packet.iPort);
	iName = packet.iName;
	iPassword = string();
	iDescription = packet.iDescription;
	iServerVersion = packet.iServerVersion;
	iPlayerCount = packet.iPlayerCount;
	iIsBilled = packet.iScoreKeeping;
	iHopCount = packet.iHopCount;
	iMinPlayerCount = std::min(iMinPlayerCount, iPlayerCount);
	iMaxPlayerCount = std::max(iMaxPlayerCount, iPlayerCount);
}

//
// A game server has sent a master update for this zone.
//
void Zone::OnUpdate(const ZoneUpdatePacket & packet)
{
	uint32 now = Time::GetMilliCount();
	auto min = packet.iPlayerCount;
	auto max = packet.iPlayerCount;

	if(iIsMaster)
	{
		if(iPassword != packet.iPassword)
		{
			iLogger.Log(KLogWarning, "Received update for zone '%s' with incorrect password.", iName.c_str());
			return;
		}

		min = std::min(iMinPlayerCount, min);
		max = std::max(iMaxPlayerCount, max);
	}

	iIsMaster = true;
	iUpdateTime = now;
	iAddress = InetAddress(Primitive::Ntohl(packet.iIp), packet.iPort);
	iName = packet.iName;
	iPassword = packet.iPassword;
	iDescription = packet.iDescription;
	iServerVersion = packet.iServerVersion;
	iPlayerCount = packet.iPlayerCount;
	iIsBilled = packet.iScoreKeeping;
	iHopCount = 0;
	iMinPlayerCount = min;
	iMaxPlayerCount = max;
}

bool Zone::IsDisabled() const
{
	return IsStale() || (iIsMaster && ((Time::GetMilliCount() - iUpdateTime) >= 5 * 60 * 1000));
}

bool Zone::IsStale() const
{
	uint32 millisSinceUpdate = (Time::GetMilliCount() - iUpdateTime);
	return (iIsMaster && millisSinceUpdate >= /*iConfig.GetPasswordReclaimTime()*/ 3600 * 24 * 5 * 1000) ||
	       (!iIsMaster && millisSinceUpdate >= 2 * 60 * 1000);
}

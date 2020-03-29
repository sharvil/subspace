/*++

Copyright (c) Sharvil Nanavati.

Module:

    Peer.cpp

Authors:

    Sharvil Nanavati (sharvil) 2010-03-15

--*/

#include "base.h"
#include "Peer.h"
#include "LogLib.h"
#include "Config.h"
#include "ZoneList.h"

#include "dirserver.proto.h"

Peer::Peer(ZoneList & zones, const Config & config, SSConnectionPool & connPool, const string & hostname)
	: iLogger(Logger::Instance()),
	  iZones(zones),
	  iConfig(config),
	  iConnectionPool(connPool),
	  iConnection(0),
	  iHostname(hostname)
{
	iLogger.Log(KLogDebug, "Added peer %s.", hostname.c_str());
}

Peer::~Peer()
{
	delete iConnection;
}

void Peer::Poll(SSConnection & connection)
{
}

void Peer::OnConnect(SSConnection & connection)
{
}

void Peer::OnDisconnect()
{
}

void Peer::OnReceive(SSConnection & connection, const bstring & packet)
{
	if(packet.empty() || packet[0] != 0x01)
		return;

	ProcessZoneList(packet);

	iConnection->Disconnect();
}

void Peer::SendNewRequest()
{
	delete iConnection;
	iConnection = 0;

	try
	{
		InetAddress remoteHost(iHostname);

		iConnection = iConnectionPool.Connect(remoteHost);
		iConnection->AddListener(*this);
		iConnection->Send(bstring("\x01\x00\x00\x00\x00", 5), true);

		iLogger.Log(KLogDebug, "Requesting zone list from peer %s.", iHostname.c_str());
	}
	catch(const not_found_error &)
	{
	}
}

void Peer::ProcessZoneList(const bstring & packet)
{
	ZonePacket::reader_t reader(packet);
	reader.ReadUint8();

	uint32 count = 0;
	uint32 totalPop = 0;
	try
	{
		while(!reader.IsEndOfData())
		{
			ZonePacket zonePacket;
			zonePacket.Deserialize(reader);
			InetAddress address = InetAddress(Primitive::Ntohl(zonePacket.iIp), zonePacket.iPort);

			if(!iConfig.IsZoneBanned(address, zonePacket.iName) && iConfig.IsAlive(zonePacket.iHopCount))
			{
				Zone * zone = iZones.Find(zonePacket.iName);
				if(zone != 0)
				{
					zone->OnListing(zonePacket);
					iLogger.Log(KLogDebug, "Peer %s updated zone %s.", iHostname.c_str(), zone->iName.c_str());
				}
				else
				{
					Zone newZone;
					newZone.OnListing(zonePacket);
					iZones.Add(newZone);
					iLogger.Log(KLogDebug, "Peer %s added zone (%d) %s.", iHostname.c_str(), newZone.iServerVersion, newZone.iName.c_str());
				}

				totalPop += zonePacket.iPlayerCount;
				++count;
			}
		}
		iLogger.Log(KLogInfo, "%s delivered %d zones with a total population of %d.", iHostname.c_str(), count, totalPop);
	}
	catch(const invalid_argument &)
	{
		iLogger.Log(KLogWarning, "Invalid response from %s after parsing %d zones.", iHostname.c_str(), count);
	}
}

bool Peer::operator < (const Peer & rhs) const
{
	return String::Compare(iHostname, rhs.iHostname) < 0;
}

/*++

Copyright (c) Sharvil Nanavati.

Module:

    DirectoryServer.cpp

Authors:

    Sharvil Nanavati (sharvil) 2010-03-15

--*/

#include "base.h"
#include "DirectoryServer.h"
#include "LogLib.h"

#include "dirserver.proto.h"

DirectoryServer::DirectoryServer()
	: iLogger(Logger::Instance()),
	  iClientConnectionPool(iSelector),
	  iServerConnectionPool(iSelector),
	  iExitFlag(false)
{
	iLogger.Log(KLogInfo, " -- Snrrrub's Directory Server 1.1 started --");
	printf(" -- Snrrrub's Directory Server 1.1 started --\n");
	printf("Press CTRL+C to exit.\n");
}

DirectoryServer::~DirectoryServer()
{
	ClearPeers();

	printf("Shutting down the directory server.\n");
	iLogger.Log(KLogInfo, "Shutting down the directory server.");
}

void DirectoryServer::Run()
{
	ReloadConfig();

	iClientConnectionPool.Bind(iConfig.GetBaseListenPort());

	iServer.Bind(iConfig.GetBaseListenPort() + 1);
	iServer.Register(iSelector, Selector::KRead);

	//
	// Start up the periodic events for writing stats and querying peers.
	//
	WriteZoneStats();
	QueryPeers();

	while(!iExitFlag)
	{
		iSelector.Select();

		if(iSelector.ReadSet().find(&iServer) != iSelector.ReadSet().end())
		{
			InetAddress source;
			bstring packet = iServer.Read(source, 1024);
			if(packet.GetLength() == 4)
			{
				bstring pong("\x01\x00\x00\x00", 4);
				pong.Append(packet);
				iServer.Write(source, pong);
			}
			else
			{
				ZoneUpdatePacket zoneUpdate;
				zoneUpdate.Deserialize(packet);

				Zone * zone = iZones.Find(zoneUpdate.iName);
				if(zone != 0)
				{
					zone->OnUpdate(zoneUpdate);
					iLogger.Log(KLogDebug, "Received master update for zone %s.", zoneUpdate.iName.c_str());
				}
				else
				{
					Zone newZone;
					newZone.OnUpdate(zoneUpdate);
					iZones.Add(newZone);
					iLogger.Log(KLogDebug, "Added new master zone %s.", zoneUpdate.iName.c_str());
				}
			}
		}
	}
}

void DirectoryServer::Exit()
{
	iExitFlag = true;
	iSelector.Alert();
}

void DirectoryServer::ReloadConfig()
{
	if(iConfig.Load())
	{
		iLogger.Log(KLogInfo, "* Configuration reloaded.");

		ClearPeers();

		auto newUpstreamServers = iConfig.GetUpstreamServerList();
		for(auto i = newUpstreamServers.begin(); i != newUpstreamServers.end(); ++i)
			iPeers.push_back(new Peer(iZones, iConfig, iServerConnectionPool, *i));
	}
	iSelector.AddEvent(std::bind(&DirectoryServer::ReloadConfig, this), iConfig.GetConfigReloadPeriod() * 1000);
}

void DirectoryServer::WriteZoneStats()
{
	iStatisticsWriter.Write(iZones);
	iSelector.AddEvent(std::bind(&DirectoryServer::WriteZoneStats, this), iConfig.GetStatisticsDumpPeriod() * 1000);
}

void DirectoryServer::QueryPeers()
{
	for(auto i = iPeers.begin(); i != iPeers.end(); ++i)
		(*i)->SendNewRequest();

	iSelector.AddEvent(std::bind(&DirectoryServer::QueryPeers, this), iConfig.GetZoneListRefreshPeriod() * 1000);
}

void DirectoryServer::ClearPeers()
{
	for(auto i = iPeers.begin(); i != iPeers.end(); ++i)
		delete *i;
	iPeers.clear();
}

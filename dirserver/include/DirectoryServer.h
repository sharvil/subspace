/*++

Copyright (c) Sharvil Nanavati.

Module:

    DirectoryServer.h

Authors:

    Sharvil Nanavati (sharvil) 2010-03-15

--*/

#pragma once

#include "base.h"
#include "NetLib.h"
#include "SSNetLib.h"
#include "TimeLib.h"
#include "Config.h"
#include "Peer.h"
#include "StatisticsWriter.h"
#include "ZoneList.h"

#include <vector>

class Logger;

class DirectoryServer : public NonCopyable
{
	public:
		DirectoryServer();
		~DirectoryServer();

		void Run();
		void Exit();

	private:
		typedef std::vector <Peer *> Peers;

		void QueryPeers();
		void ClearPeers();
		void ReloadConfig();
		void WriteZoneStats();

		Logger & iLogger;
		Selector iSelector;
		UdpClient iServer;
		SSConnectionPool iClientConnectionPool;
		SSConnectionPool iServerConnectionPool;

		Config iConfig;
		Peers iPeers;
		ZoneList iZones;
		StatisticsWriter iStatisticsWriter;

		bool iExitFlag;
};

/*++

Copyright (c) Sharvil Nanavati.

Module:

    Peer.h

Authors:

    Sharvil Nanavati (sharvil) 2010-03-15

--*/

#pragma once

#include "base.h"
#include "SSNetLib.h"

class ZoneList;
class Logger;
class Config;

class Peer : public SSConnection::IListener
{
	public:
		explicit Peer(ZoneList & zones, const Config & config, SSConnectionPool & connPool, const string & hostname);
		~Peer();

		void SendNewRequest();

		bool operator < (const Peer & rhs) const;

	private:
		void Poll(SSConnection & connection);
		void OnConnect(SSConnection & connection);
		void OnReceive(SSConnection & connection, const bstring & packet);
		void OnDisconnect();

		void ProcessZoneList(const bstring & packet);

		Logger & iLogger;
		ZoneList & iZones;
		const Config & iConfig;
		SSConnectionPool & iConnectionPool;
		SSConnection * iConnection;
		string iHostname;
};

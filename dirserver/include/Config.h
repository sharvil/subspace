/*++

Copyright (c) 2010 Sharvil Nanavati.

Module:

    Config.h

Authors:

    Sharvil Nanavati (sharvil) 2010-05-06

--*/

#pragma once

#include "base.h"
#include "NetLib.h"

class Logger;
class ILogSink;

class Config : public NonCopyable
{
	public:
		Config();
		~Config();

		bool Load();
		void RestoreDefaults();

		const StringISet & GetUpstreamServerList() const;
		uint32 GetPasswordReclaimTime() const;
		uint32 GetZoneListRefreshPeriod() const;
		uint32 GetStatisticsDumpPeriod() const;
		uint32 GetConfigReloadPeriod() const;
		uint16 GetBaseListenPort() const;

		bool IsZoneBanned(const InetAddress & ip, const string & name) const;
		bool IsAlive(uint32 hopCount) const;

	private:
		typedef std::list <CidrNetmask> NetmaskList;

		bool IsZoneIpBanned(const InetAddress & ip) const;
		bool IsZoneNameBanned(const string & name) const;

		static const string KConfigFile;

		Logger & iLogger;
		ILogSink * iFileLogSink;

		StringISet iUpstreamServers;
		uint32 iPasswordReclaimSeconds;
		uint32 iZoneListDownloadSeconds;
		uint32 iStatisticsDumpSeconds;
		uint32 iConfigReloadSeconds;
		uint16 iBaseListenPort;
		StringISet iZoneBanPrefixes;
		StringISet iZoneBanStrings;
		NetmaskList iZoneBanNetmasks;
		uint32 iTtl;

		uint32 iLastModifiedTime;
};

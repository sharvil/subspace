/*++

Copyright (c) 2010 Sharvil Nanavati.

Module:

    Config.cpp

Authors:

    Sharvil Nanavati (sharvil) 2010-05-06

--*/

#include "base.h"
#include "Config.h"
#include "LogLib.h"

#include <cstring>

const string Config::KConfigFile("config.txt");

Config::Config()
	: iLogger(Logger::Instance()),
	  iFileLogSink(0)
{
	RestoreDefaults();
}

Config::~Config()
{
	delete iFileLogSink;
}

bool Config::Load()
{
	try
	{
		uint32 fileTime = File::LastModifiedTime(KConfigFile);
		if(fileTime == iLastModifiedTime)
			return false;

		File fp(KConfigFile, "rt");

		RestoreDefaults();
		iLastModifiedTime = fileTime;

		while(!fp.IsEndOfFile())
		{
			string line = fp.ReadLine();
			string::size_type pos = line.find('#');

			if(pos != string::npos)
				line.erase(pos);

			pos = line.find('=');
			if(pos == string::npos)
				continue;

			string key = line.substr(0, pos);
			string value = line.substr(pos + 1);

			if(String::Trim(key).empty() || String::Trim(value).empty())
				continue;

			if(String::Compare(key, "UpstreamServers", String::KCaseInsensitive) == 0)
			{
				iUpstreamServers.clear();

				StringSequence servers = String::Split(value, ',');
				for(StringSequence::iterator i = servers.begin(); i != servers.end(); ++i)
					if(!String::Trim(*i).empty())
					{
						if(i->find(':') == string::npos)
							iUpstreamServers.insert(*i + ":4990");
						else
							iUpstreamServers.insert(*i);
					}
			}
			else if(String::Compare(key, "PasswordReclaimSeconds", String::KCaseInsensitive) == 0)
			{
				uint32 tmp;
				if(sscanf(value.c_str(), "%u", &tmp) == 1)
					iPasswordReclaimSeconds = tmp;
			}
			else if(String::Compare(key, "ZoneRefreshSeconds", String::KCaseInsensitive) == 0)
			{
				uint32 tmp;
				if(sscanf(value.c_str(), "%u", &tmp) == 1)
					iZoneListDownloadSeconds = tmp;
			}
			else if(String::Compare(key, "StatisticsWriteSeconds", String::KCaseInsensitive) == 0)
			{
				uint32 tmp;
				if(sscanf(value.c_str(), "%u", &tmp) == 1)
					iStatisticsDumpSeconds = tmp;
			}
			else if(String::Compare(key, "BaseListenPort", String::KCaseInsensitive) == 0)
			{
				uint32 port;
				if(sscanf(value.c_str(), "%u", &port) == 1 && port < 65536)
					iBaseListenPort = port;
			}
			else if(String::Compare(key, "TimeToLive", String::KCaseInsensitive) == 0)
			{
				uint32 ttl;
				if(sscanf(value.c_str(), "%u", &ttl) == 1)
					iTtl = std::min(32U, ttl);
			}
			else if(String::Compare(key, "ZoneBanPrefix", String::KCaseInsensitive) == 0)
			{
				StringSequence prefixes = String::Split(value, ',');
				for(StringSequence::iterator i = prefixes.begin(); i != prefixes.end(); ++i)
					if(!String::ToLower(String::Trim(*i)).empty())
						iZoneBanPrefixes.insert(*i);
			}
			else if(String::Compare(key, "ZoneBanString", String::KCaseInsensitive) == 0)
			{
				StringSequence strings = String::Split(value, ',');
				for(StringSequence::iterator i = strings.begin(); i != strings.end(); ++i)
					if(!String::ToLower(String::Trim(*i)).empty())
						iZoneBanStrings.insert(*i);
			}
			else if(String::Compare(key, "ZoneBanNetmask", String::KCaseInsensitive) == 0)
			{
				StringSequence netmasks = String::Split(value, ',');
				for(StringSequence::iterator i = netmasks.begin(); i != netmasks.end(); ++i)
					try
					{
						if(!String::Trim(*i).empty())
							iZoneBanNetmasks.push_back(CidrNetmask(*i));
					}
					catch(const std::runtime_error & error)
					{
						iLogger.Log(KLogWarning, "%s", error.what());
					}
			}
			else if(String::Compare(key, "LogFile", String::KCaseInsensitive) == 0)
			{
				try
				{
					iFileLogSink = new FileLogSink(value);
					iLogger.AttachSink(*iFileLogSink);
				}
				catch(const not_found_error &)
				{
					iLogger.Log(KLogWarning, "Unable to open log file %s.\n", value.c_str());
				}
			}
			else if(String::Compare(key, "LogDebug", String::KCaseInsensitive) == 0)
			{
				uint32 debug = 0;
				if(sscanf(value.c_str(), "%u", &debug) == 1 && debug != 0)
					iLogger.SetLogThreshold(KLogDebug);
			}
		}
	}
	catch(const not_found_error & error)
	{
		if(iLastModifiedTime == 0)
			return false;

		RestoreDefaults();
	}
	return true;
}

void Config::RestoreDefaults()
{
	delete iFileLogSink;
	iFileLogSink = 0;
	iLogger.SetLogThreshold(KLogInfo);

	iUpstreamServers.clear();
	iUpstreamServers.insert("sscentral.sscuservers.net:4990");
	iUpstreamServers.insert("ssdir.playsubspace.com:4990");
	iUpstreamServers.insert("warpath.bluetoast.org:4990");

	iPasswordReclaimSeconds = 3600 * 24;
	iZoneListDownloadSeconds = 60;
	iStatisticsDumpSeconds = 60;
	iConfigReloadSeconds = 10;
	iLastModifiedTime = 0;
	iBaseListenPort = 4990;
	iTtl = 5;
	iZoneBanPrefixes.clear();
	iZoneBanStrings.clear();
	iZoneBanNetmasks.clear();
}

const StringISet & Config::GetUpstreamServerList() const
{
	return iUpstreamServers;
}

uint32 Config::GetPasswordReclaimTime() const
{
	return iPasswordReclaimSeconds;
}

uint32 Config::GetZoneListRefreshPeriod() const
{
	return iZoneListDownloadSeconds;
}

uint32 Config::GetStatisticsDumpPeriod() const
{
	return iStatisticsDumpSeconds;
}

uint32 Config::GetConfigReloadPeriod() const
{
	return iConfigReloadSeconds;
}

uint16 Config::GetBaseListenPort() const
{
	return iBaseListenPort;
}

bool Config::IsZoneBanned(const InetAddress & ip, const string & name) const
{
	return IsZoneIpBanned(ip) || IsZoneNameBanned(name);
}

bool Config::IsZoneIpBanned(const InetAddress & ip) const
{
	for(NetmaskList::const_iterator i = iZoneBanNetmasks.begin(); i != iZoneBanNetmasks.end(); ++i)
		if(ip << *i)
			return true;

	return false;
}

bool Config::IsZoneNameBanned(const string & name) const
{
	string canonicalName = String::ToLowerCopy(name);

	for(StringISet::const_iterator i = iZoneBanPrefixes.begin(); i != iZoneBanPrefixes.end(); ++i)
		if(memcmp(canonicalName.c_str(), i->c_str(), std::min(canonicalName.length(), i->length())) == 0)
			return true;

	for(StringISet::const_iterator i = iZoneBanStrings.begin(); i != iZoneBanStrings.end(); ++i)
		if(canonicalName.find(*i) != string::npos)
			return true;

	return false;
}

bool Config::IsAlive(uint32 hopCount) const
{
	return hopCount < iTtl;
}

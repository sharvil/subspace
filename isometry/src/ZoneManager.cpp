#include "base.h"
#include "ZoneManager.h"
#include "Zone.h"
#include "SSNetLib.h"
#include "LogLib.h"

ZoneManager & ZoneManager::Instance()
{
	static ZoneManager self;
	return self;
}

ZoneManager::ZoneManager()
{
}

void ZoneManager::OnAccept(SSConnection * connection)
{
	Zone * zone = new Zone(*connection);
	connection->AddListener(*zone);
}

void ZoneManager::Register(Zone & zone)
{
	ZoneMap::iterator i = iZones.find(zone.GetServerId());
	if(i != iZones.end())
	{
		Logger::Instance().Log(KLogInfo, "[ZoneManager::Register]: duplicate zone %s registered, kicking old zone.", zone.GetName().c_str());
		i->second->Disconnect();
		iZones.erase(i);
	}

	iZones.insert(std::make_pair(zone.GetServerId(), &zone));
}

void ZoneManager::Unregister(Zone & zone)
{
	ZoneMap::iterator i = iZones.find(zone.GetServerId());
	if(i == iZones.end())
	{
		Logger::Instance().Log(KLogDebug, "[ZoneManager::Unregister]: could not find zone %s.", zone.GetName().c_str());
		return;
	}
	else if(i->second != &zone)
		return;

	iZones.erase(i);
}

ZoneManager::iterator ZoneManager::begin() const
{
	return iZones.begin();
}

ZoneManager::iterator ZoneManager::end() const
{
	return iZones.end();
}

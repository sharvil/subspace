#pragma once

#include "base.h"
#include "Zone.h"

#include <map>

class ISSProtocol;

class ZoneManager : public NonCopyable, public SSConnectionPool::IListener
{
	private:
		typedef std::map <uint32, Zone *> ZoneMap;

	public:
		friend class Zone;
		typedef ZoneMap::const_iterator iterator;

		static ZoneManager & Instance();

		iterator begin() const;
		iterator end() const;

	private:
		ZoneManager();

		void OnAccept(SSConnection * connection);

		void Register(Zone & zone);
		void Unregister(Zone & zone);

		ZoneMap iZones;
};

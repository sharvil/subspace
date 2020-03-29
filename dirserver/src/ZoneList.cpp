/*++

Copyright (c) 2010 Sharvil Nanavati.

Module:

    ZoneList.cpp

Authors:

    Sharvil Nanavati (sharvil) 2010-05-18

--*/

#include "base.h"
#include "ZoneList.h"
#include "LogLib.h"

void ZoneList::Add(const Zone & zone)
{
	ClearStaleZones();

	iZones.insert(std::make_pair(zone.iName, zone));
}

Zone * ZoneList::Find(const string & name)
{
	ClearStaleZones();

	auto i = iZones.find(name);
	if(i == iZones.end())
		return 0;

	return &i->second;
}

ZoneList::iterator ZoneList::begin() const
{
	return iterator(iZones.begin());
}

ZoneList::iterator ZoneList::end() const
{
	return iterator(iZones.end());
}

void ZoneList::ClearStaleZones()
{
	for(auto i = iZones.begin(); i != iZones.end(); )
	{
		auto here = i++;
		if(here->second.IsStale())
			iZones.erase(here);
	}
}

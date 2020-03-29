/*++

Copyright (c) 2010 Sharvil Nanavati.

Module:

    ZoneList.h

Authors:

    Sharvil Nanavati (sharvil) 2010-05-18

--*/

#pragma once

#include "base.h"
#include "Zone.h"
#include "iterator_adapter.h"

#include <map>

class Config;

class ZoneList
{
	private:
		typedef std::map <string, Zone> container_t;

	public:
		typedef const_value_iterator <container_t> iterator;

		void Add(const Zone & zone);
		Zone * Find(const string & name);

		iterator begin() const;
		iterator end() const;

	private:
		void ClearStaleZones();

		container_t iZones;
};

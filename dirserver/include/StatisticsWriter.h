/*++

Copyright (c) 2010 Sharvil Nanavati.

Module:

    StatisticsWriter.h

Authors:

    Sharvil Nanavati (sharvil) 2010-05-06

--*/

#pragma once

#include "base.h"
#include "LogLib.h"

class ZoneList;

class StatisticsWriter
{
	public:
		StatisticsWriter();

		void Write(const ZoneList & zones);

	private:
		void WriteModern(const ZoneList & zones);
		void WriteTraditional(const ZoneList & zones);
		void WritePopulations(const ZoneList & zones);

		typedef std::vector <uint32> PopHistory;

		Logger & iLogger;
		const Time iServerStartTime;

		PopHistory iHistory;
};

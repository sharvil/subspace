/*++

Copyright (c) 2010 Sharvil Nanavati.

Module:

    StatisticsWriter.cpp

Authors:

    Sharvil Nanavati (sharvil) 2010-05-06

--*/

#include "base.h"
#include "StatisticsWriter.h"
#include "ZoneList.h"
#include "Zone.h"

#include <sstream>

StatisticsWriter::StatisticsWriter()
	: iLogger(Logger::Instance())
{
}

void StatisticsWriter::Write(const ZoneList & zones)
{
	WriteModern(zones);
	WritePopulations(zones);
	WriteTraditional(zones);
}

void StatisticsWriter::WriteModern(const ZoneList & zones)
{
	iLogger.Log(KLogDebug, "Writing zone statistics.");

	try
	{
		File fp("stats/current.xml", "wt");

		std::stringstream content;
		content << "<DirectoryServer>"
		        << "<startTime>"
		        << iServerStartTime.ToLocalTzString()
		        << "</startTime>";

		for(auto i = zones.begin(); i != zones.end(); ++i)
		{
			if(i->IsDisabled())
				continue;

			content << "<zone>"
			        << "<name>"
			        << UrlEncoding::DeserializeString(bstring(i->iName))
			        << "</name>"
			        << "<description>"
			        << UrlEncoding::DeserializeString(bstring(i->iDescription))
			        << "</description>"
			        << "<population>"
			        << i->iPlayerCount
			        << "</population>"
			        << "<maxPopulation>"
			        << i->iMaxPlayerCount
			        << "</maxPopulation>"
			        << "<minPopulation>"
			        << i->iMinPlayerCount
			        << "</minPopulation>"
			        << "</zone>";
		}

		content << "</DirectoryServer>";

		fp.Write(bstring(content.str()));
	}
	catch(const std::runtime_error & error)
	{
		iLogger.Log(KLogError, "Error writing stats/current.xml: %s", error.what());
	}
}

void StatisticsWriter::WritePopulations(const ZoneList & zones)
{
	iLogger.Log(KLogDebug, "Writing population distribution statistics.");

	while(iHistory.size() >= 24 * 60 / 1)
		iHistory.erase(iHistory.begin());

	uint32 population = 0;
	for(auto i = zones.begin(); i != zones.end(); ++i)
		if(!i->IsDisabled())
			population += i->iPlayerCount;
	iHistory.push_back(population);

	try
	{
		File fp("stats/distribution.xml", "wt");

		std::stringstream content;
		content << "<DirectoryServer>"
		        << "<startTime>"
		        << iServerStartTime.ToLocalTzString()
		        << "</startTime>"
		        << "<distribution>";

		for(PopHistory::iterator i = iHistory.begin(); i != iHistory.end(); ++i)
		{
			content << "<sample>"
			        << *i
			        << "</sample>";
		}

		content << "</distribution>";
		content << "</DirectoryServer>";

		fp.Write(bstring(content.str()));
	}
	catch(const std::runtime_error & error)
	{
		iLogger.Log(KLogError, "Error writing stats/distribution.xml: %s", error.what());
	}
}

void StatisticsWriter::WriteTraditional(const ZoneList & zones)
{
	iLogger.Log(KLogDebug, "Writing traditional statistics.");

	try
	{
		File fp("stats/stats.txt", "wt");

		std::stringstream content;
		for(auto i = zones.begin(); i != zones.end(); ++i)
		{
			if(i->IsDisabled())
				continue;

			content << i->iAddress.GetIpAddress() << "\t"
			        << i->iAddress.GetPort() << "\t"
			        << i->iPlayerCount << "\t"
			        << (i->iIsBilled ? 1 : 0) << "\t"
			        << i->iName << "\t"
			        << i->iDescription << std::endl;
		}

		fp.Write(bstring(content.str()));
	}
	catch(const std::runtime_error & error)
	{
		iLogger.Log(KLogError, "Error writing stats/stats.txt: %s", error.what());
	}
}

#pragma once

#include "base.h"

struct Ban
{
	bool iIsActive;
	uint32 iId;
	uint32 iAccess;
	std::string iSysop;
	std::string iBanDate;
	uint32 iBanDays;
	std::string iIpRange;
	std::string iPlayerName;
	std::string iComment;
};

struct BanData
{
	BanData()
	{
		for(uint32 i = 0; i < 14; ++i)
			iValues[i] = 0;
	}

	std::string AsSqlArray() const
	{
		std::string banData;
		for(uint32 i = 0; i < 13; ++i)
			banData += Format("%d,", iValues[i]);
		banData += Format("%d", iValues[13]);

		return banData;
	}

	uint32 iValues[14];
};

#pragma once

#include "base.h"
#include "DbLib.h"
#include "Exceptions.h"
#include "Ban.h"

namespace Database
{
	class PlayerGetLastLoginInfo : public DbAction
	{
		public:
			PlayerGetLastLoginInfo(const std::string & name, bool netwide, uint32 zoneId, uint32 & id, InetAddress & ipAddress, uint32 & machineId, uint32 & timezonebias, BanData & bandata)
				: iName(name),
				  iNetwide(netwide),
				  iZoneId(zoneId),
				  iId(id),
				  iIpAddress(ipAddress),
				  iMachineId(machineId),
				  iTimeZoneBias(timezonebias),
				  iBanData(bandata)
			{ }

			void operator()(Transaction & t)
			{
				std::string query;
				if(iNetwide)
					query = Format("select id, ipaddress, machineid, timezonebias, bandata from isometry.player, isometry.loginhistory where lower(name)='%s' and playerid=id order by logintime desc limit 1", esc(String::ToLowerCopy(iName)).c_str());
				else
					query = Format("select id, ipaddress, machineid, timezonebias, bandata from isometry.player, isometry.loginhistory where lower(name)='%s' and playerid=id and zoneid=%d order by logintime desc limit 1", esc(String::ToLowerCopy(iName)).c_str(), iZoneId);
				pqxx::result res = t.exec(query);

				if(res.empty())
					throw player_not_found_error(iName);

				std::string ip;

				res[0][0].to(iId);
				res[0][1].to(ip);
				res[0][2].to(iMachineId);
				res[0][3].to(iTimeZoneBias);

				std::string banData;
				res[0][4].to(banData);

				uint32 i = 0, here = 1, hereAccum;
				while(i < 14 && sscanf(banData.c_str() + here, " %d %n", &iBanData.iValues[i], &hereAccum) >= 1)
				{
					here += hereAccum + 1;
					++i;
				}

				iIpAddress = InetAddress(ip);
			}

		private:
			std::string iName;
			bool iNetwide;
			uint32 iZoneId;

			uint32 & iId;
			InetAddress & iIpAddress;
			uint32 & iMachineId;
			uint32 & iTimeZoneBias;
			BanData & iBanData;
	};
}

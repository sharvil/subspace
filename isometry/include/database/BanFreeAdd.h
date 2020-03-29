#pragma once

#include "base.h"
#include "DbLib.h"

namespace Database
{
	class BanFreeAdd : public DbAction
	{
		public:
			explicit BanFreeAdd(uint32 sysopId, bool netwide, uint32 zoneId, uint32 access, const std::string & playerName)
				: iSysopId(sysopId),
				  iNetwide(netwide),
				  iZoneId(zoneId),
				  iAccess(access),
				  iPlayerName(playerName)
			{ }

			void operator()(Transaction & t)
			{
				std::string query = Format("select id from isometry.player where lower(name)='%s'", esc(String::ToLowerCopy(iPlayerName)).c_str());
				pqxx::result res = t.exec(query);

				if(res.empty())
					throw player_not_found_error(iPlayerName);

				uint32 playerId;
				res[0][0].to(playerId);

				query = Format("select cannetban, access from isometry.opgroup, isometry.globalop where id=groupid and playerid=%d", iSysopId);
				res = t.exec(query);

				bool canNetBan = false;
				uint32 access = ~0;
				if(!res.empty())
				{
					res[0][0].to(canNetBan);
					res[0][1].to(access);
				}

				if(access > iAccess || (iNetwide && !canNetBan))
				{
					query = Format("select cannetban, access from isometry.opgroup, isometry.localop where id=groupid and playerid=%d and zoneid=%d", iSysopId, iZoneId);
					res = t.exec(query);
					if(!res.empty())
					{
						res[0][0].to(canNetBan);
						res[0][1].to(access);
					}

					if(access > iAccess || (iNetwide && !canNetBan))
						throw permission_error("cannot ban free");
				}

				if(iNetwide)
				{
					query = Format("select true from isometry.banfree where playerid=%d and zoneid is null", playerId);
					res = t.exec(query);
					if(!res.empty())
						throw constraint_error("ban free", "duplicate");

					query = Format("insert into isometry.banfree (freedby, playerid, access) values (%d, %d, %d)", iSysopId, playerId, iAccess);
				}
				else
					query = Format("insert into isometry.banfree (freedby, playerid, zoneid, access) values (%d, %d, %d, %d)", iSysopId, playerId, iZoneId, iAccess);

				t.exec(query);
			}

		private:
			uint32 iSysopId;
			bool iNetwide;
			uint32 iZoneId;
			uint32 iAccess;
			const std::string & iPlayerName;
	};
}

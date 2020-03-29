#pragma once

#include "base.h"
#include "DbLib.h"

namespace Database
{
	class BanFreeRemove : public DbAction
	{
		public:
			explicit BanFreeRemove(uint32 sysopId, bool netwide, uint32 zoneId, const std::string & playerName)
				: iSysopId(sysopId),
				  iNetwide(netwide),
				  iZoneId(zoneId),
				  iPlayerName(playerName)
			{ }

			void operator()(Transaction & t)
			{
				std::string query = Format("select p.id, b.access from isometry.player as p, isometry.banfree as b where lower(p.name)='%s' and p.id=b.playerid and ", esc(String::ToLowerCopy(iPlayerName)).c_str());
				if(iNetwide)
					query += "b.zoneid is null";
				else
					query += Format("b.zoneid=%d", iZoneId);
				pqxx::result res = t.exec(query);

				if(res.empty())
					throw player_not_found_error(iPlayerName);

				uint32 playerId;
				uint32 targetAccess;
				res[0][0].to(playerId);
				res[0][1].to(targetAccess);

				query = Format("select cannetban, access from isometry.opgroup, isometry.globalop where id=groupid and playerid=%d", iSysopId);
				res = t.exec(query);

				bool canNetBan = false;
				uint32 access = ~0;
				if(!res.empty())
				{
					res[0][0].to(canNetBan);
					res[0][1].to(access);
				}

				if(access > targetAccess || (iNetwide && !canNetBan))
				{
					query = Format("select cannetban, access from isometry.opgroup, isometry.localop where id=groupid and playerid=%d and zoneid=%d", iSysopId, iZoneId);
					res = t.exec(query);
					if(!res.empty())
					{
						res[0][0].to(canNetBan);
						res[0][1].to(access);
					}

					if(access > targetAccess || (iNetwide && !canNetBan))
						throw permission_error("cannot remove ban free");
				}

				if(iNetwide)
					query = Format("delete from isometry.banfree where playerid=%d and zoneid is null", playerId);
				else
					query = Format("delete from isometry.banfree where playerid=%d and zoneid=%d", playerId, iZoneId);

				t.exec(query);
			}

		private:
			uint32 iSysopId;
			bool iNetwide;
			uint32 iZoneId;
			const std::string & iPlayerName;
	};
}

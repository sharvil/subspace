#pragma once

#include "base.h"
#include "DbLib.h"
#include "Ban.h"
#include "Exceptions.h"

namespace Database
{
	class BanList : public DbAction
	{
		private:
			typedef std::list <Ban> container;

		public:
			typedef container::const_iterator iterator;

			explicit BanList(uint32 sysopId, uint32 zoneId, bool netwide, uint32 limit)
				: iSysopId(sysopId),
				  iZoneId(zoneId),
				  iNetwide(netwide),
				  iLimit(limit)
			{ }

			void operator()(Transaction & t)
			{
				std::string query = Format("select true from isometry.opgroup, isometry.globalop where playerid=%d and id=groupid", iSysopId);
				pqxx::result res = t.exec(query);

				if(res.empty())
				{
					query = Format("select true from isometry.opgroup, isometry.localop where playerid=%d and zoneid=%d and id=groupid", iSysopId, iZoneId);
					res = t.exec(query);

					if(res.empty())
						throw permission_error("not an operator");
				}

				if(iNetwide)
					query = Format("select now() < b.bantime+b.banperiod, b.id, b.access, s.name, date_trunc('second', b.bantime), trunc(extract(epoch from b.banperiod)/3600/24), b.ipaddress, p.name, b.comment from isometry.ban as b, isometry.player as s, isometry.player as p where b.zoneid is null and b.bannedby=s.id and b.playerid=p.id order by bantime desc limit %d", iLimit);
				else
					query = Format("select now() < b.bantime+b.banperiod, b.id, b.access, s.name, date_trunc('second', b.bantime), trunc(extract(epoch from b.banperiod)/3600/24), b.ipaddress, p.name, b.comment from isometry.ban as b, isometry.player as s, isometry.player as p where b.zoneid=%d and b.bannedby=s.id and b.playerid=p.id order by bantime desc limit %d", iZoneId, iLimit);

				res = t.exec(query);

				for(uint32 i = 0; i < res.size(); ++i)
				{
					Ban ban;
					res[i][0].to(ban.iIsActive);
					res[i][1].to(ban.iId);
					res[i][2].to(ban.iAccess);
					res[i][3].to(ban.iSysop);
					res[i][4].to(ban.iBanDate);
					res[i][5].to(ban.iBanDays);
					res[i][6].to(ban.iIpRange);
					res[i][7].to(ban.iPlayerName);

					if(!res[i][8].is_null())
						res[i][8].to(ban.iComment);

					iBans.push_front(ban);
				}
			}

			iterator begin() const { return iBans.begin(); }
			iterator end() const { return iBans.end(); }
			bool empty() { return iBans.empty(); }

		private:
			uint32 iSysopId;
			uint32 iZoneId;
			bool iNetwide;
			uint32 iLimit;

			container iBans;
	};
}

#pragma once

#include "base.h"
#include "DbLib.h"
#include "BanFree.h"
#include "Exceptions.h"

namespace Database
{
	class BanFreeList : public DbAction
	{
		private:
			typedef std::list <BanFree> container;

		public:
			typedef container::const_iterator iterator;

			explicit BanFreeList(uint32 sysopId, uint32 zoneId, bool netwide, uint32 limit)
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
					query = Format("select date_trunc('second', b.freetime), s.name, p.name, b.access from isometry.banfree as b, isometry.player as s, isometry.player as p where b.zoneid is null and b.freedby=s.id and b.playerid=p.id order by freetime desc limit %d", iLimit);
				else
					query = Format("select date_trunc('second', b.freetime), s.name, p.name, b.access from isometry.banfree as b, isometry.player as s, isometry.player as p where b.zoneid=%d and b.freedby=s.id and b.playerid=p.id order by freetime desc limit %d", iZoneId, iLimit);

				res = t.exec(query);

				for(uint32 i = 0; i < res.size(); ++i)
				{
					BanFree ban;
					res[i][0].to(ban.iTime);
					res[i][1].to(ban.iGranter);
					res[i][2].to(ban.iName);
					res[i][3].to(ban.iAccess);

					iBanFrees.push_front(ban);
				}
			}

			iterator begin() const { return iBanFrees.begin(); }
			iterator end() const { return iBanFrees.end(); }
			bool empty() { return iBanFrees.empty(); }

		private:
			uint32 iSysopId;
			uint32 iZoneId;
			bool iNetwide;
			uint32 iLimit;

			container iBanFrees;
	};
}

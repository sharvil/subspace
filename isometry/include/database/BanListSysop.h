#pragma once

#include "base.h"
#include "DbLib.h"
#include "Operator.h"
#include "Exceptions.h"

namespace Database
{
	class BanListSysop : public DbAction
	{
		private:
			typedef std::list <Operator> container;

		public:
			typedef container::const_iterator iterator;

			explicit BanListSysop(uint32 sysopId, uint32 zoneId, bool netwide)
				: iSysopId(sysopId),
				  iZoneId(zoneId),
				  iNetwide(netwide)
			{ }

			void operator()(Transaction & t)
			{
				std::string query = Format("select cannetban from isometry.opgroup, isometry.globalop where playerid=%d and id=groupid", iSysopId);
				pqxx::result res = t.exec(query);

				bool canNetBan = false;
				if(!res.empty())
					res[0][0].to(canNetBan);

				if(res.empty() || (iNetwide && !canNetBan))
				{
					query = Format("select cannetban from isometry.opgroup, isometry.localop where playerid=%d and zoneid=%d and id=groupid", iSysopId, iZoneId);
					res = t.exec(query);

					if(!res.empty())
						res[0][0].to(canNetBan);

					if(res.empty() || (iNetwide && !canNetBan))
						throw permission_error("not operator or not privileged enough");
				}

				if(iNetwide)
					query = Format("select o.access, p.name from isometry.opgroup as o, isometry.player as p, isometry.globalop as g where g.playerid=p.id and g.groupid=o.id order by o.access asc");
				else
					query = Format("select o.access, p.name from isometry.opgroup as o, isometry.player as p, isometry.localop as g where g.zoneid=%d and g.playerid=p.id and g.groupid=o.id order by o.access asc", iZoneId);

				res = t.exec(query);

				for(uint32 i = 0; i < res.size(); ++i)
				{
					Operator o;
					res[i][0].to(o.iAccess);
					res[i][1].to(o.iName);

					iSysops.push_back(o);
				}
			}

			iterator begin() const { return iSysops.begin(); }
			iterator end() const { return iSysops.end(); }
			bool empty() { return iSysops.empty(); }

		private:
			uint32 iSysopId;
			uint32 iZoneId;
			bool iNetwide;

			container iSysops;
	};
}

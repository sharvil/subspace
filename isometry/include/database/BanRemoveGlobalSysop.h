#pragma once

#include "base.h"
#include "DbLib.h"

namespace Database
{
	class BanRemoveGlobalSysop : public DbAction
	{
		public:
			explicit BanRemoveGlobalSysop(uint32 granterId, uint32 zoneId, const std::string & playerName)
				: iGranterId(granterId),
				  iZoneId(zoneId),
				  iPlayerName(playerName)
			{ }

			void operator()(Transaction & t)
			{
				std::string query = Format("select g.access, p.id from isometry.globalop, isometry.opgroup as g, isometry.player as p where lower(p.name)='%s' and playerid=p.id and g.id=groupid", esc(String::ToLowerCopy(iPlayerName)).c_str());
				pqxx::result res = t.exec(query);

				if(res.empty())
					throw op_not_found_error(iPlayerName);

				uint32 targetAccess;
				uint32 playerId;
				res[0][0].to(targetAccess);
				res[0][1].to(playerId);

				query = Format("select addglobalops, access from isometry.opgroup, isometry.globalop where playerid=%d and groupid=id", iGranterId);
				res = t.exec(query);

				uint32 access = ~0;
				bool addGlobalOps = false;
				if(!res.empty())
				{
					res[0][0].to(addGlobalOps);
					res[0][1].to(access);
				}

				if(!addGlobalOps || access >= targetAccess)
				{
					query = Format("select addglobalops, access from isometry.opgroup, isometry.localop where playerid=%d and zoneid=%d and groupid=id", iGranterId, iZoneId);
					res = t.exec(query);

					if(!res.empty())
					{
						res[0][0].to(addGlobalOps);
						res[0][1].to(access);
					}

					if(!addGlobalOps || access >= targetAccess)
						throw permission_error("not enough permission to remove global op");
				}

				query = Format("delete from isometry.globalop where playerid=%d", playerId);
				t.exec(query);
			}

		private:
			uint32 iGranterId;
			uint32 iZoneId;
			const std::string & iPlayerName;
	};
}

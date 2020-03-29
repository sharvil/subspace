#pragma once

#include "base.h"
#include "DbLib.h"

namespace Database
{
	class BanRemoveLocalSysop : public DbAction
	{
		public:
			explicit BanRemoveLocalSysop(uint32 granterId, uint32 zoneId, const std::string & playerName)
				: iGranterId(granterId),
				  iZoneId(zoneId),
				  iPlayerName(playerName)
			{ }

			void operator()(Transaction & t)
			{
				std::string query = Format("select g.access, p.id from isometry.localop, isometry.opgroup as g, isometry.player as p where lower(p.name)='%s' and p.id=playerid and zoneid=%d and g.id=groupid", esc(String::ToLowerCopy(iPlayerName)).c_str(), iZoneId);
				pqxx::result res = t.exec(query);

				if(res.empty())
					throw op_not_found_error(iPlayerName);

				uint32 targetAccess;
				uint32 playerId;
				res[0][0].to(targetAccess);
				res[0][1].to(playerId);

				query = Format("select addlocalops, access from isometry.opgroup, isometry.globalop where playerid=%d and groupid=id", iGranterId);
				res = t.exec(query);

				uint32 access = ~0;
				bool addLocalOps = false;
				if(!res.empty())
				{
					res[0][0].to(addLocalOps);
					res[0][1].to(access);
				}

				if(!addLocalOps || access >= targetAccess)
				{
					query = Format("select addlocalops, access from isometry.opgroup, isometry.localop where playerid=%d and zoneid=%d and groupid=id", iGranterId, iZoneId);
					res = t.exec(query);

					if(!res.empty())
					{
						res[0][0].to(addLocalOps);
						res[0][1].to(access);
					}

					if(!addLocalOps || access >= targetAccess)
						throw permission_error("not enough permission to remove local op");
				}

				query = Format("delete from isometry.localop where playerid=%d and zoneid=%d", playerId, iZoneId);
				t.exec(query);
			}

		private:
			uint32 iGranterId;
			uint32 iZoneId;
			const std::string & iPlayerName;
	};
}

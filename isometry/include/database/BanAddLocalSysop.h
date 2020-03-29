#pragma once

#include "base.h"
#include "DbLib.h"

namespace Database
{
	class BanAddLocalSysop : public DbAction
	{
		public:
			explicit BanAddLocalSysop(uint32 granterId, uint32 zoneId, const std::string & playerName, const std::string & opGroup)
				: iGranterId(granterId),
				  iZoneId(zoneId),
				  iPlayerName(playerName),
				  iOpGroup(opGroup)
			{ }

			void operator()(Transaction & t)
			{
				std::string query = Format("select id, access from isometry.opgroup where name='%s'", esc(iOpGroup).c_str());
				pqxx::result res = t.exec(query);

				if(res.empty())
					throw op_group_not_found_error(iOpGroup);

				uint32 groupId;
				uint32 targetAccess;
				res[0][0].to(groupId);
				res[0][1].to(targetAccess);

				query = Format("select addlocalops, access from isometry.opgroup, isometry.globalop where playerid=%d and id=groupid", iGranterId);
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
						throw permission_error("not enough permission to add local op");
				}

				query = Format("insert into isometry.localop (playerid, zoneid, groupid) select id, %d, %d from isometry.player where lower(name)='%s'", iZoneId, groupId, esc(String::ToLowerCopy(iPlayerName)).c_str());
				res = t.exec(query);

				if(res.affected_rows() == 0)
					throw player_not_found_error(iPlayerName);
			}

		private:
			uint32 iGranterId;
			uint32 iZoneId;
			std::string iPlayerName;
			std::string iOpGroup;
	};
}

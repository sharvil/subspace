#pragma once

#include "base.h"
#include "DbLib.h"
#include "Exceptions.h"

namespace Database
{
	class SquadDeny : public DbAction
	{
		public:
			explicit SquadDeny(uint32 squadId, uint32 ownerId, uint32 denyerId, const std::string & player)
				: iSquadId(squadId),
				  iOwnerId(ownerId),
				  iDenyerId(denyerId),
				  iPlayerName(player)
			{ }

			void operator()(Transaction & t)
			{
				if(iOwnerId != iDenyerId)
				{
					std::string query = Format("select true from isometry.squadpermission where squadid=%d and allow=true and playerid=%d", iSquadId, iDenyerId);
					pqxx::result res = t.exec(query);

					if(res.empty())
						throw permission_error("no permission to deny player from squad.");
				}

				std::string query = Format("select id from isometry.player where lower(name)='%s'", esc(String::ToLowerCopy(iPlayerName)).c_str());
				pqxx::result res = t.exec(query);

				if(res.empty())
					throw player_not_found_error(iPlayerName);

				uint32 playerId;
				res[0][0].to(playerId);

				query = Format("delete from isometry.squadpermission where squadid=%d and playerid=%d", iSquadId, playerId);
				t.exec(query);

				query = Format("update isometry.player set squad=null where id=%d and squad=%d", playerId, iSquadId);
				t.exec(query);
			}

		private:
			uint32 iSquadId;
			uint32 iOwnerId;
			uint32 iDenyerId;
			const std::string & iPlayerName;
	};
}

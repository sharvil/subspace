#pragma once

#include "base.h"
#include "DbLib.h"
#include "Exceptions.h"

namespace Database
{
	class SquadAllow : public DbAction
	{
		public:
			explicit SquadAllow(uint32 squadId, uint32 ownerId, uint32 granterId, const std::string & player)
				: iSquadId(squadId),
				  iOwnerId(ownerId),
				  iGranterId(granterId),
				  iPlayerName(player)
			{ }

			void operator()(Transaction & t)
			{
				if(iOwnerId != iGranterId)
				{
					std::string query = Format("select true from isometry.squadpermission where squadid=%d and allow=true and playerid=%d", iSquadId, iGranterId);
					pqxx::result res = t.exec(query);

					if(res.empty())
						throw permission_error("no permission to allow player into squad.");
				}

				std::string query = Format("select id from isometry.player where lower(name)='%s'", esc(String::ToLowerCopy(iPlayerName)).c_str());
				pqxx::result res = t.exec(query);

				if(res.empty())
					throw player_not_found_error(iPlayerName);

				uint32 playerId;
				res[0][0].to(playerId);

				query = Format("insert into isometry.squadpermission (squadid, playerid) values (%d, %d)", iSquadId, playerId);
				t.exec(query);
			}

		private:
			uint32 iSquadId;
			uint32 iOwnerId;
			uint32 iGranterId;
			const std::string & iPlayerName;
	};
}

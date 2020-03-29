#pragma once

#include "base.h"
#include "DbLib.h"
#include "Exceptions.h"

namespace Database
{
	class SquadGrant : public DbAction
	{
		public:
			explicit SquadGrant(uint32 squadId, uint32 ownerId, uint32 granterId, const std::string & player, uint32 & newOwnerId)
				: iSquadId(squadId),
				  iOwnerId(ownerId),
				  iGranterId(granterId),
				  iPlayerName(player),
				  iNewOwnerId(newOwnerId)
			{ }

			void operator()(Transaction & t)
			{
				if(iOwnerId != iGranterId)
					throw permission_error("no permission to grant squad.");

				std::string query = Format("update isometry.squad set owner=p.id from isometry.player as p where lower(p.name)='%s' returning p.id", esc(String::ToLowerCopy(iPlayerName)).c_str());
				pqxx::result res = t.exec(query);

				if(res.empty())
					throw player_not_found_error(iPlayerName);

				res[0][0].to(iNewOwnerId);
			}

		private:
			uint32 iSquadId;
			uint32 iOwnerId;
			uint32 iGranterId;
			const std::string & iPlayerName;
			uint32 & iNewOwnerId;
	};
}

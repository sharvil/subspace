#pragma once

#include "base.h"
#include "DbLib.h"
#include "Exceptions.h"

namespace Database
{
	class SquadJoin : public DbAction
	{
		public:
			explicit SquadJoin(uint32 squadId, bool isPublic, uint32 ownerId, uint32 playerId)
				: iSquadId(squadId),
				  iIsPublic(isPublic),
				  iOwnerId(ownerId),
				  iPlayerId(playerId)
			{ }

			void operator()(Transaction & t)
			{
				if(!iIsPublic && iOwnerId != iPlayerId)
				{
					std::string query = Format("select true from isometry.squadpermission where squadid=%d and playerid=%d", iSquadId, iPlayerId);
					pqxx::result res = t.exec(query);

					if(res.empty())
						throw permission_error("no permission to join squad");
				}

				std::string query = Format("update isometry.player set squad=%d where id=%d", iSquadId, iPlayerId);
				t.exec(query);
			}

		private:
			uint32 iSquadId;
			bool iIsPublic;
			uint32 iOwnerId;
			uint32 iPlayerId;
	};
}

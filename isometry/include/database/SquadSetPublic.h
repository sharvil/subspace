#pragma once

#include "base.h"
#include "DbLib.h"
#include "Exceptions.h"

namespace Database
{
	class SquadSetPublic : public DbAction
	{
		public:
			explicit SquadSetPublic(uint32 squadId, uint32 playerId, bool isPublic)
				: iSquadId(squadId),
				  iPlayerId(playerId),
				  iIsPublic(isPublic)
			{ }

			void operator()(Transaction & t)
			{
				std::string query = Format("update isometry.squad set public=%s where id=%d and owner=%d", (iIsPublic ? "true" : "false"), iSquadId, iPlayerId);
				pqxx::result res = t.exec(query);

				if(res.affected_rows() == 0)
					throw permission_error("no permission to change squad mode");
			}

		private:
			uint32 iSquadId;
			uint32 iPlayerId;
			bool iIsPublic;
	};
}

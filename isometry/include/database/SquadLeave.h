#pragma once

#include "base.h"
#include "DbLib.h"
#include "Exceptions.h"

namespace Database
{
	class SquadLeave : public DbAction
	{
		public:
			explicit SquadLeave(uint32 squadId, uint32 playerId)
				: iSquadId(squadId),
				  iPlayerId(playerId)
			{ }

			void operator()(Transaction & t)
			{
				std::string query = Format("update isometry.player set squad=null where id=%d and squad=%d", iPlayerId, iSquadId);
				t.exec(query);
			}

		private:
			uint32 iSquadId;
			uint32 iPlayerId;
	};
}

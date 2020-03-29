#pragma once

#include "base.h"
#include "DbLib.h"
#include "Exceptions.h"

namespace Database
{
	class SquadDelete : public DbAction
	{
		public:
			explicit SquadDelete(uint32 squadId, uint32 playerId)
				: iSquadId(squadId),
				  iPlayerId(playerId)
			{ }

			void operator()(Transaction & t)
			{
				std::string query = Format("delete from isometry.squad where id=%d and owner=%d", iSquadId, iPlayerId);
				t.exec(query);
			}

		private:
			uint32 iSquadId;
			uint32 iPlayerId;
	};
}

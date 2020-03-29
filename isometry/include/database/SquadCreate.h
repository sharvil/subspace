#pragma once

#include "base.h"
#include "DbLib.h"
#include "Exceptions.h"

namespace Database
{
	class SquadCreate : public DbAction
	{
		public:
			explicit SquadCreate(const std::string & name, uint32 playerId, uint32 & squadId)
				: iName(name),
				  iPlayerId(playerId),
				  iSquadId(squadId)
			{ }

			void operator()(Transaction & t)
			{
				std::string query = Format("insert into isometry.squad (name, owner) values ('%s', %d) returning id", esc(iName).c_str(), iPlayerId);
				pqxx::result res = t.exec(query);

				if(res.empty())
					throw system_error("error inserting squad into database");

				res[0][0].to(iSquadId);
			}

		private:
			const std::string & iName;
			uint32 iPlayerId;
			uint32 & iSquadId;
	};
}

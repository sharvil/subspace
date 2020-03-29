#pragma once

#include "base.h"
#include "DbLib.h"
#include "Exceptions.h"

#include <list>

namespace Database
{
	class PlayerListSquads : public DbAction
	{
		private:
			typedef std::list <std::string> container;

		public:
			typedef container::const_iterator iterator;

			explicit PlayerListSquads(uint32 playerId)
				: iPlayerId(playerId)
			{ }

			void operator()(Transaction & t)
			{
				std::string query = Format("select name from isometry.squad where owner=%d order by name", iPlayerId);
				pqxx::result res = t.exec(query);

				for(uint32 i = 0; i < res.size(); ++i)
				{
					std::string name;
					res[i][0].to(name);
					iSquads.push_back(name);
				}
			}

			iterator begin() const { return iSquads.begin(); }
			iterator end() const { return iSquads.end(); }
			bool empty() { return iSquads.empty(); }

		private:
			uint32 iPlayerId;
			container iSquads;
	};
}

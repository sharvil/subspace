#pragma once

#include "base.h"
#include "DbLib.h"
#include "Exceptions.h"

#include <list>

namespace Database
{
	class PlayerListChats : public DbAction
	{
		private:
			typedef std::list <std::string> container;

		public:
			typedef container::const_iterator iterator;

			explicit PlayerListChats(uint32 playerId)
				: iPlayerId(playerId)
			{ }

			void operator()(Transaction & t)
			{
				std::string query = Format("select name from isometry.chat where owner=%d order by name", iPlayerId);
				pqxx::result res = t.exec(query);

				for(uint32 i = 0; i < res.size(); ++i)
				{
					std::string name;
					res[i][0].to(name);
					iChats.push_back(name);
				}
			}

			iterator begin() const { return iChats.begin(); }
			iterator end() const { return iChats.end(); }
			bool empty() { return iChats.empty(); }

		private:
			uint32 iPlayerId;
			container iChats;
	};
}

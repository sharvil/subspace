#pragma once

#include "base.h"
#include "DbLib.h"
#include "Exceptions.h"

#include <vector>

namespace Database
{
	class ScoreGetTop10SquadDeaths : public DbAction
	{
		private:
			struct Item
			{
				std::string iName;
				uint32 iDeaths;
			};
			typedef std::vector <Item> container;

		public:
			typedef container::const_iterator iterator;

			explicit ScoreGetTop10SquadDeaths(uint32 scoreId)
				: iScoreId(scoreId)
			{ }

			void operator()(Transaction & t)
			{
				std::string query = Format("select sq.name, sum(s.losses) from isometry.squad as sq, isometry.player as p, isometry.score as s where sq.id=p.squad and s.scoreid=%d and s.playerid=p.id and p.squad is not null group by sq.name order by sum(s.losses) desc, sq.name asc limit 10", iScoreId);
				pqxx::result res = t.exec(query);

				for(uint32 i = 0; i < res.size(); ++i)
				{
					Item item;
					res[i][0].to(item.iName);
					res[i][1].to(item.iDeaths);

					iItems.push_back(item);
				}
			}

			iterator begin() const { return iItems.begin(); }
			iterator end() const { return iItems.end(); }

		private:
			uint32 iScoreId;
			std::vector <Item> iItems;
	};
}

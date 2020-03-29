#pragma once

#include "base.h"
#include "DbLib.h"
#include "Exceptions.h"

#include <vector>

namespace Database
{
	class ScoreGetTop10SquadPoints : public DbAction
	{
		private:
			struct Item
			{
				std::string iName;
				uint32 iPoints;
			};
			typedef std::vector <Item> container;

		public:
			typedef container::const_iterator iterator;

			explicit ScoreGetTop10SquadPoints(uint32 scoreId)
				: iScoreId(scoreId)
			{ }

			void operator()(Transaction & t)
			{
				std::string query = Format("select sq.name, sum(s.points+s.flagpoints) from isometry.squad as sq, isometry.player as p, isometry.score as s where sq.id=p.squad and s.scoreid=0 and s.playerid=p.id and p.squad is not null group by sq.name order by sum(s.points+s.flagpoints) desc, sq.name asc limit 10", iScoreId);
				pqxx::result res = t.exec(query);

				for(uint32 i = 0; i < res.size(); ++i)
				{
					Item item;
					res[i][0].to(item.iName);
					res[i][1].to(item.iPoints);

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

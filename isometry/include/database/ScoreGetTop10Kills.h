#pragma once

#include "base.h"
#include "DbLib.h"
#include "Exceptions.h"

#include <vector>

namespace Database
{
	class ScoreGetTop10Kills : public DbAction
	{
		private:
			struct Item
			{
				std::string iName;
				uint32 iKills;
			};
			typedef std::vector <Item> container;

		public:
			typedef container::const_iterator iterator;

			explicit ScoreGetTop10Kills(uint32 scoreId)
				: iScoreId(scoreId)
			{ }

			void operator()(Transaction & t)
			{
				std::string query = Format("select p.name, s.wins from isometry.player as p, isometry.score as s where p.id=s.playerid and s.scoreid=%d order by s.wins desc, p.name asc limit 10", iScoreId);
				pqxx::result res = t.exec(query);

				for(uint32 i = 0; i < res.size(); ++i)
				{
					Item item;
					res[i][0].to(item.iName);
					res[i][1].to(item.iKills);

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
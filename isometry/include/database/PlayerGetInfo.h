#pragma once

#include "base.h"
#include "DbLib.h"
#include "Exceptions.h"

#include <cstring>

namespace Database
{
	class PlayerGetInfo : public DbAction
	{
		public:
			PlayerGetInfo(const std::string & name, uint32 scoreid)
				: iName(name),
				  iScoreId(scoreid),
				  iHasSquad(false)
			{ }

			void operator()(Transaction & t)
			{
				std::string query = Format("select id, creationdate, secondsplayed, squad, banner from isometry.player where lower(name)='%s'", esc(String::ToLowerCopy(iName)).c_str());
				pqxx::result res = t.exec(query);

				if(res.empty())
					throw player_not_found_error(iName);

				res[0][0].to(iPlayerId);
				res[0][1].to(iCreationDate);
				res[0][2].to(iSecondsPlayed);
				if(!res[0][3].is_null())
				{
					iHasSquad = true;
					res[0][3].to(iSquadId);
				}

				if(!res[0][4].is_null())
				{
					pqxx::binarystring s(res[0][4]);
					iBanner.Assign(s.c_ptr(), s.size());
				}
				else
				{
					iBanner.Resize(96);
					memset(&iBanner[0], 0, 96);
				}

				query = Format("select wins, losses, goals, points, flagPoints from isometry.score where scoreid=%d and playerid=%d", iScoreId, iPlayerId);
				res = t.exec(query);

				if(res.empty())
				{
					query = Format("insert into isometry.score (scoreid, playerid) values (%d, %d)", iScoreId, iPlayerId);
					t.exec(query);

					iWins = iLosses = iGoals = iPoints = iFlagPoints = 0;
				}
				else
				{
					res[0][0].to(iWins);
					res[0][1].to(iLosses);
					res[0][2].to(iGoals);
					res[0][3].to(iPoints);
					res[0][4].to(iFlagPoints);
				}
			}

			uint32 GetId() const { return iPlayerId; }
			const std::string & GetCreationDate() const { return iCreationDate; }
			uint32 GetSecondsPlayed() const { return iSecondsPlayed; }
			uint32 GetWins() const { return iWins; }
			uint32 GetLosses() const { return iLosses; }
			uint32 GetGoals() const { return iGoals; }
			uint32 GetPoints() const { return iPoints; }
			uint32 GetFlagPoints() const { return iFlagPoints; }
			bool HasSquad() const { return iHasSquad; }
			uint32 GetSquadId() const { return iSquadId; }
			bstring GetBanner() const { return iBanner; }

		private:
			std::string iName;
			uint32 iScoreId;

			uint32 iPlayerId;
			std::string iCreationDate;
			uint32 iSecondsPlayed;
			uint32 iWins;
			uint32 iLosses;
			uint32 iGoals;
			uint32 iPoints;
			uint32 iFlagPoints;
			bstring iBanner;
			bool iHasSquad;
			uint32 iSquadId;
	};
}

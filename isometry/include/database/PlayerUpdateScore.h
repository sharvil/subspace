#pragma once

#include "base.h"
#include "DbLib.h"
#include "Exceptions.h"

namespace Database
{
	class PlayerUpdateScore : public DbAction
	{
		public:
			PlayerUpdateScore(uint32 scoreid, uint32 playerid, uint32 wins, uint32 losses, uint32 goals, uint32 points, uint32 flagPoints)
				: iScoreId(scoreid),
				  iPlayerId(playerid),
				  iWins(wins),
				  iLosses(losses),
				  iGoals(goals),
				  iPoints(points),
				  iFlagPoints(flagPoints)
			{ }

			void operator()(Transaction & t)
			{
				std::string query = Format("update isometry.score set wins=%d, losses=%d, goals=%d, points=%d, flagpoints=%d where scoreid=%d and playerid=%d", iWins, iLosses, iGoals, iPoints, iFlagPoints, iScoreId, iPlayerId);
				t.exec(query);
			}

		private:
			uint32 iScoreId;
			uint32 iPlayerId;
			uint32 iWins;
			uint32 iLosses;
			uint32 iGoals;
			uint32 iPoints;
			uint32 iFlagPoints;
	};
}

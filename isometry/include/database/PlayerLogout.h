#pragma once

#include "base.h"
#include "DbLib.h"
#include "Exceptions.h"

namespace Database
{
	class PlayerLogout : public DbAction
	{
		public:
			PlayerLogout(uint32 id, uint32 secondsPlayed)
				: iId(id),
				  iSecondsPlayed(secondsPlayed)
			{ }

			void operator()(Transaction & t)
			{
				std::string query = Format("update isometry.player set lastseendate=now(), logincount=logincount+1, secondsplayed=secondsplayed+%d where id=%d", iSecondsPlayed, iId);
				t.exec(query);
			}

		private:
			uint32 iId;
			uint32 iSecondsPlayed;
	};
}

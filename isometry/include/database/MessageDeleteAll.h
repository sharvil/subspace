#pragma once

#include "base.h"
#include "DbLib.h"
#include "Exceptions.h"

namespace Database
{
	class MessageDeleteAll : public DbAction
	{
		public:
			explicit MessageDeleteAll(uint32 playerId)
				: iPlayerId(playerId)
			{ }

			void operator()(Transaction & t)
			{
				std::string query = Format("delete from isometry.message where dest=%d", iPlayerId);
				t.exec(query);
			}

		private:
			uint32 iPlayerId;
	};
}

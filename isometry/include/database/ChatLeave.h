#pragma once

#include "base.h"
#include "DbLib.h"
#include "Exceptions.h"

namespace Database
{
	class ChatLeave : public DbAction
	{
		public:
			explicit ChatLeave(uint32 chatId, uint32 playerId)
				: iChatId(chatId),
				  iPlayerId(playerId)
			{ }

			void operator()(Transaction & t)
			{
				std::string query = Format("delete from isometry.chatsubscription where playerid=%d and chatid=%d", iPlayerId, iChatId);
				t.exec(query);
			}

		private:
			uint32 iChatId;
			uint32 iPlayerId;
	};
}

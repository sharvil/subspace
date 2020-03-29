#pragma once

#include "base.h"
#include "DbLib.h"
#include "Exceptions.h"

namespace Database
{
	class ChatDelete : public DbAction
	{
		public:
			explicit ChatDelete(uint32 chatId, uint32 playerId)
				: iChatId(chatId),
				  iPlayerId(playerId)
			{ }

			void operator()(Transaction & t)
			{
				std::string query = Format("delete from isometry.chat where id=%d and owner=%d", iChatId, iPlayerId);
				t.exec(query);
			}

		private:
			uint32 iChatId;
			uint32 iPlayerId;
	};
}

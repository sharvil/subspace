#pragma once

#include "base.h"
#include "DbLib.h"
#include "Exceptions.h"

namespace Database
{
	class ChatCreate : public DbAction
	{
		public:
			explicit ChatCreate(const std::string & name, uint32 playerId, uint32 & chatId)
				: iName(name),
				  iPlayerId(playerId),
				  iChatId(chatId)
			{ }

			void operator()(Transaction & t)
			{
				std::string query = Format("insert into isometry.chat (name, owner) values ('%s', %d) returning id", esc(iName).c_str(), iPlayerId);
				pqxx::result res = t.exec(query);

				if(res.empty())
					throw system_error("could not create chat");

				res[0][0].to(iChatId);
			}

		private:
			const std::string & iName;
			uint32 iPlayerId;
			uint32 & iChatId;
	};
}

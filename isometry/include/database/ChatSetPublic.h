#pragma once

#include "base.h"
#include "DbLib.h"
#include "Exceptions.h"

namespace Database
{
	class ChatSetPublic : public DbAction
	{
		public:
			explicit ChatSetPublic(uint32 chatId, uint32 playerId, bool isPublic)
				: iChatId(chatId),
				  iPlayerId(playerId),
				  iIsPublic(isPublic)
			{ }

			void operator()(Transaction & t)
			{
				std::string query = Format("update isometry.chat set public=%s where id=%d and owner=%d", (iIsPublic ? "true" : "false"), iChatId, iPlayerId);
				pqxx::result res = t.exec(query);

				if(res.affected_rows() == 0)
					throw permission_error("no permission to change chat mode");
			}

		private:
			uint32 iChatId;
			uint32 iPlayerId;
			bool iIsPublic;
	};
}

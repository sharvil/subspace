#pragma once

#include "base.h"
#include "DbLib.h"
#include "Exceptions.h"

namespace Database
{
	class ChatJoin : public DbAction
	{
		public:
			explicit ChatJoin(uint32 chatId, bool isPublic, uint32 ownerId, uint32 playerId, uint32 ordinal)
				: iChatId(chatId),
				  iIsPublic(isPublic),
				  iOwnerId(ownerId),
				  iPlayerId(playerId),
				  iOrdinal(ordinal)
			{ }

			void operator()(Transaction & t)
			{
				if(!iIsPublic && iOwnerId != iPlayerId)
				{
					std::string query = Format("select true from isometry.chatpermission where chatid=%d and playerid=%d", iChatId, iPlayerId);
					pqxx::result res = t.exec(query);

					if(res.empty())
						throw permission_error("no permission to join chat");
				}

				std::string query = Format("update isometry.chatsubscription set chatid=%d where playerid=%d and ordinal=%d and chatid<>%d", iChatId, iPlayerId, iOrdinal, iChatId);
				pqxx::result res = t.exec(query);

				if(res.affected_rows() == 0)
				{
					query = Format("insert into isometry.chatsubscription (chatid, playerid, ordinal) values (%d, %d, %d)", iChatId, iPlayerId, iOrdinal);
					t.exec(query);
				}
			}

		private:
			uint32 iChatId;
			bool iIsPublic;
			uint32 iOwnerId;
			uint32 iPlayerId;
			uint32 iOrdinal;
	};
}

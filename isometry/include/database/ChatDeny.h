#pragma once

#include "base.h"
#include "DbLib.h"
#include "Exceptions.h"

namespace Database
{
	class ChatDeny : public DbAction
	{
		public:
			explicit ChatDeny(uint32 chatId, uint32 ownerId, uint32 denyerId, const std::string & player)
				: iChatId(chatId),
				  iOwnerId(ownerId),
				  iDenyerId(denyerId),
				  iPlayerName(player)
			{ }

			void operator()(Transaction & t)
			{
				if(iOwnerId != iDenyerId)
				{
					std::string query = Format("select true from isometry.chatpermission where chatid=%d and allow=true and playerid=%d", iChatId, iDenyerId);
					pqxx::result res = t.exec(query);

					if(res.empty())
						throw permission_error("no permission to deny player from chat.");
				}

				std::string query = Format("select id from isometry.player where lower(name)='%s'", esc(String::ToLowerCopy(iPlayerName)).c_str());
				pqxx::result res = t.exec(query);

				if(res.empty())
					throw player_not_found_error(iPlayerName);

				uint32 playerId;
				res[0][0].to(playerId);

				query = Format("delete from isometry.chatpermission where chatid=%d and playerid=%d", iChatId, playerId);
				t.exec(query);

				query = Format("delete from isometry.chatsubscription where chatid=%d and playerid=%d", iChatId, playerId);
				t.exec(query);
			}

		private:
			uint32 iChatId;
			uint32 iOwnerId;
			uint32 iDenyerId;
			const std::string & iPlayerName;
	};
}

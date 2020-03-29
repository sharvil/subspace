#pragma once

#include "base.h"
#include "DbLib.h"
#include "Exceptions.h"

namespace Database
{
	class ChatAllow : public DbAction
	{
		public:
			explicit ChatAllow(uint32 chatId, uint32 ownerId, uint32 granterId, const std::string & player)
				: iChatId(chatId),
				  iOwnerId(ownerId),
				  iGranterId(granterId),
				  iPlayerName(player)
			{ }

			void operator()(Transaction & t)
			{
				if(iOwnerId != iGranterId)
				{
					std::string query = Format("select true from isometry.chatpermission where chatid=%d and allow=true and playerid=%d", iChatId, iGranterId);
					pqxx::result res = t.exec(query);

					if(res.empty())
						throw permission_error("no permission to allow player into chat.");
				}

				std::string query = Format("select id from isometry.player where lower(name)='%s'", esc(String::ToLowerCopy(iPlayerName)).c_str());
				pqxx::result res = t.exec(query);

				if(res.empty())
					throw player_not_found_error(iPlayerName);

				uint32 playerId;
				res[0][0].to(playerId);

				query = Format("insert into isometry.chatpermission (chatid, playerid) values (%d, %d)", iChatId, playerId);
				t.exec(query);
			}

		private:
			uint32 iChatId;
			uint32 iOwnerId;
			uint32 iGranterId;
			const std::string & iPlayerName;
	};
}

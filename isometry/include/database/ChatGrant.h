#pragma once

#include "base.h"
#include "DbLib.h"
#include "Exceptions.h"

namespace Database
{
	class ChatGrant : public DbAction
	{
		public:
			explicit ChatGrant(uint32 chatId, uint32 ownerId, uint32 granterId, const std::string & player, uint32 & newOwnerId)
				: iChatId(chatId),
				  iOwnerId(ownerId),
				  iGranterId(granterId),
				  iPlayerName(player),
				  iNewOwnerId(newOwnerId)
			{ }

			void operator()(Transaction & t)
			{
				if(iOwnerId != iGranterId)
					throw permission_error("no permission to grant chat.");

				std::string query = Format("update isometry.chat set owner=p.id from isometry.player as p where lower(p.name)='%s' returning p.id", esc(String::ToLowerCopy(iPlayerName)).c_str());
				pqxx::result res = t.exec(query);

				if(res.empty())
					throw player_not_found_error(iPlayerName);

				res[0][0].to(iNewOwnerId);
			}

		private:
			uint32 iChatId;
			uint32 iOwnerId;
			uint32 iGranterId;
			const std::string & iPlayerName;
			uint32 & iNewOwnerId;
	};
}

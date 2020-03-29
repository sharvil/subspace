#pragma once

#include "base.h"
#include "DbLib.h"
#include "Exceptions.h"

namespace Database
{
	class ChatGetInfo : public DbAction
	{
		public:
			explicit ChatGetInfo(uint32 & chatId, std::string & name, uint32 & ownerId, std::string & ownerName, bool & isPublic)
				: iChatId(chatId),
				  iName(name),
				  iOwnerId(ownerId),
				  iOwnerName(ownerName),
				  iIsPublic(isPublic)
			{ }

			void operator()(Transaction & t)
			{
				std::string query = "select p.id, p.name, c.id, c.name, c.public from isometry.player as p, isometry.chat as c where p.id=c.owner and ";
				if(iName.empty())
					query += Format("c.id=%d", iChatId);
				else
					query += Format("lower(c.name)='%s'", esc(String::ToLowerCopy(iName)).c_str());
				pqxx::result res = t.exec(query);

				if(res.empty())
					throw chat_not_found_error(iName);

				res[0][0].to(iOwnerId);
				res[0][1].to(iOwnerName);
				res[0][2].to(iChatId);
				res[0][3].to(iName);
				res[0][4].to(iIsPublic);
			}

		private:
			uint32 & iChatId;
			std::string & iName;
			uint32 & iOwnerId;
			std::string & iOwnerName;
			bool & iIsPublic;
	};
}

#pragma once

#include "base.h"
#include "DbLib.h"
#include "Exceptions.h"

#include <list>

namespace Database
{
	class ChatGetMemberList : public DbAction
	{
		private:
			typedef std::list <std::string> container;

		public:
			typedef container::const_iterator iterator;

			explicit ChatGetMemberList(uint32 chatId)
				: iChatId(chatId)
			{ }

			void operator()(Transaction & t)
			{
				std::string query = Format("select p.name from isometry.player as p, isometry.chatsubscription as c where c.chatid=%d and c.playerid=p.id", iChatId);
				pqxx::result res = t.exec(query);

				for(uint32 i = 0; i < res.size(); ++i)
				{
					std::string name;
					res[i][0].to(name);
					iMembers.push_back(name);
				}
			}

			iterator begin() const { return iMembers.begin(); }
			iterator end() const { return iMembers.end(); }
			bool empty() { return iMembers.empty(); }

		private:
			uint32 iChatId;
			container iMembers;
	};
}

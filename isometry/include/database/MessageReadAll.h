#pragma once

#include "base.h"
#include "DbLib.h"
#include "Message.h"
#include "Exceptions.h"

namespace Database
{
	class MessageReadAll : public DbAction
	{
		private:
			typedef std::list <Message> container;

		public:
			typedef container::const_iterator iterator;

			explicit MessageReadAll(uint32 playerId)
				: iPlayerId(playerId)
			{ }

			void operator()(Transaction & t)
			{
				std::string query = Format("select date_trunc('second', time), name, message from isometry.message as m, isometry.player as p where m.dest=%d and m.source=p.id order by time asc", iPlayerId);
				pqxx::result res = t.exec(query);

				for(uint32 i = 0; i < res.size(); ++i)
				{
					Message m;
					res[i][0].to(m.iTime);
					res[i][1].to(m.iFrom);
					res[i][2].to(m.iMessage);

					iMessages.push_back(m);
				}
			}

			iterator begin() const { return iMessages.begin(); }
			iterator end() const { return iMessages.end(); }
			bool empty() { return iMessages.empty(); }

		private:
			uint32 iPlayerId;
			container iMessages;
	};
}

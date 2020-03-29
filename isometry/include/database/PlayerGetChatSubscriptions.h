#pragma once

#include "base.h"
#include "DbLib.h"
#include "Exceptions.h"

#include <vector>

namespace Database
{
	class PlayerGetChatSubscriptions : public DbAction
	{
		public:
			explicit PlayerGetChatSubscriptions(uint32 playerId)
				: iPlayerId(playerId)
			{ }

			void operator()(Transaction & t)
			{
				std::string query = Format("select ordinal, chatid from isometry.chatsubscription where playerid=%d", iPlayerId);
				pqxx::result res = t.exec(query);

				for(uint32 i = 0; i < res.size(); ++i)
				{
					uint32 ordinal, chatid;
					res[i][0].to(ordinal);
					res[i][1].to(chatid);

					if(ordinal >= iSubscriptions.size())
						iSubscriptions.resize(ordinal + 1);

					iSubscriptions[ordinal] = chatid;
				}
			}

			bool HasSubscription(uint32 ordinal) const
			{
				if(ordinal < 0 || ordinal >= iSubscriptions.size())
					return false;

				return iSubscriptions[ordinal] != 0;
			}

			uint32 GetSubscription(uint32 ordinal) const
			{
				if(ordinal < 0 || ordinal >= iSubscriptions.size())
					return 0;

				return iSubscriptions[ordinal];
			}

		private:
			uint32 iPlayerId;
			std::vector <uint32> iSubscriptions;
	};
}

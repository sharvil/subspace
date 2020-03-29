#pragma once

#include "base.h"
#include "DbLib.h"
#include "Exceptions.h"

#include <list>

namespace Database
{
	class PlayerGetPermission : public DbAction
	{
		public:
			explicit PlayerGetPermission(uint32 playerId, uint32 zoneId, bool & canSendBillerMessage)
				: iPlayerId(playerId),
				  iZoneId(zoneId),
				  iCanSendBillerMessage(canSendBillerMessage)
			{ }

			void operator()(Transaction & t)
			{
				std::string query = Format("select g.cansendbillermessage from isometry.globalop as o, isometry.opgroup as g where o.playerid=%d and o.groupid=g.id", iPlayerId);
				pqxx::result res = t.exec(query);

				if(!res.empty())
					res[0][0].to(iCanSendBillerMessage);

				if(!iCanSendBillerMessage)
				{
					query = Format("select g.cansendbillermessage from isometry.localop as o, isometry.opgroup as g where o.zoneid=%d and o.playerid=%d and o.groupid=g.id", iZoneId, iPlayerId);
					res = t.exec(query);

					if(!res.empty())
						res[0][0].to(iCanSendBillerMessage);
				}
			}

		private:
			uint32 iPlayerId;
			uint32 iZoneId;
			bool & iCanSendBillerMessage;
	};
}

#pragma once

#include "base.h"
#include "DbLib.h"
#include "Exceptions.h"

namespace Database
{
	class SquadGetInfo : public DbAction
	{
		public:
			explicit SquadGetInfo(uint32 & squadId, std::string & name, uint32 & ownerId, std::string & ownerName, bool & isPublic)
				: iSquadId(squadId),
				  iName(name),
				  iOwnerId(ownerId),
				  iOwnerName(ownerName),
				  iIsPublic(isPublic)
			{ }

			void operator()(Transaction & t)
			{
				std::string query = "select p.id, p.name, s.id, s.name, s.public from isometry.player as p, isometry.squad as s where p.id=s.owner and ";
				if(iName.empty())
					query += Format("s.id=%d", iSquadId);
				else
					query += Format("lower(s.name)='%s'", esc(String::ToLowerCopy(iName)).c_str());
				pqxx::result res = t.exec(query);

				if(res.empty())
					throw squad_not_found_error(iName);

				res[0][0].to(iOwnerId);
				res[0][1].to(iOwnerName);
				res[0][2].to(iSquadId);
				res[0][3].to(iName);
				res[0][4].to(iIsPublic);
			}

		private:
			uint32 & iSquadId;
			std::string & iName;
			uint32 & iOwnerId;
			std::string & iOwnerName;
			bool & iIsPublic;
	};
}

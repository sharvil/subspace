#pragma once

#include "base.h"
#include "DbLib.h"
#include "Exceptions.h"

#include <list>

namespace Database
{
	class SquadGetMemberList : public DbAction
	{
		private:
			typedef std::list <std::string> container;

		public:
			typedef container::const_iterator iterator;

			explicit SquadGetMemberList(uint32 squadId)
				: iSquadId(squadId)
			{ }

			void operator()(Transaction & t)
			{
				std::string query = Format("select name from isometry.player where squad=%d", iSquadId);
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
			uint32 iSquadId;
			container iMembers;
	};
}

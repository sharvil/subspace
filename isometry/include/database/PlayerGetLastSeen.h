#pragma once

#include "base.h"
#include "DbLib.h"
#include "Exceptions.h"

#include <list>

namespace Database
{
	class PlayerGetLastSeen : public DbAction
	{
		public:
			explicit PlayerGetLastSeen(const std::string & name, std::string & lastSeen)
				: iName(name),
				  iLastSeen(lastSeen)
			{ }

			void operator()(Transaction & t)
			{
				std::string query = Format("select now()-lastseendate from isometry.player where lower(name)='%s'", esc(String::ToLowerCopy(iName)).c_str());
				pqxx::result res = t.exec(query);

				if(res.empty())
					throw player_not_found_error(iName);

				res[0][0].to(iLastSeen);
			}

		private:
			const std::string & iName;
			std::string & iLastSeen;
	};
}

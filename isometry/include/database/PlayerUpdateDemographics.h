#pragma once

#include "base.h"
#include "DbLib.h"
#include "Exceptions.h"

namespace Database
{
	class PlayerUpdateDemographics : public DbAction
	{
		public:
			PlayerUpdateDemographics(uint32 id, const std::string & emailaddress)
				: iId(id),
				  iEmailAddress(emailaddress)
			{ }

			void operator()(Transaction & t)
			{
				std::string query = Format("update isometry.player set email='%s' where id=%d", esc(iEmailAddress).c_str(), iId);
				t.exec(query);
			}

		private:
			uint32 iId;
			const std::string & iEmailAddress;
	};
}

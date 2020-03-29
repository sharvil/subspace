#pragma once

#include "base.h"
#include "DbLib.h"
#include "Exceptions.h"

namespace Database
{
	class ZoneLogout : public DbAction
	{
		public:
			explicit ZoneLogout(uint32 id)
				: iId(id)
			{ }

			void operator()(Transaction & t)
			{
				std::string query = Format("update isometry.zone set lastactivedate=now() where id=%d", iId);
				t.exec(query);
			}

		private:
			uint32 iId;
	};
}

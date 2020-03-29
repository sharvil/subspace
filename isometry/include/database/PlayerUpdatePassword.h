#pragma once

#include "base.h"
#include "DbLib.h"
#include "Exceptions.h"

namespace Database
{
	class PlayerUpdatePassword : public DbAction
	{
		public:
			PlayerUpdatePassword(uint32 id, const std::string & password)
				: iId(id),
				  iPassword(password)
			{ }

			void operator()(Transaction & t)
			{
				std::string query = Format("update isometry.player set password='%s' where id=%d", esc(iPassword).c_str(), iId);
				t.exec(query);
			}

		private:
			uint32 iId;
			const std::string & iPassword;
	};
}

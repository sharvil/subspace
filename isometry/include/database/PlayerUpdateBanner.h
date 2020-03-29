#pragma once

#include "base.h"
#include "DbLib.h"
#include "Exceptions.h"

namespace Database
{
	class PlayerUpdateBanner : public DbAction
	{
		public:
			PlayerUpdateBanner(uint32 id, const bstring & banner)
				: iId(id),
				  iBanner(banner)
			{ }

			void operator()(Transaction & t)
			{
				//std::string query = Format("update isometry.player set banner=E'%s' where id=%d", esc_raw(iBanner.Data(), iBanner.GetLength()).c_str(), iId);
				std::string query = Format("update isometry.player set banner=E'%s' where id=%d", ""/*esc(iBanner.Data(), iBanner.GetLength()).c_str()*/, iId);
				t.exec(query);
			}

		private:
			uint32 iId;
			const bstring & iBanner;
	};
}

#pragma once

#include "base.h"
#include "DbLib.h"
#include "Exceptions.h"

namespace Database
{
	class ZoneUpdateBanText : public DbAction
	{
		public:
			explicit ZoneUpdateBanText(uint32 sysopId, uint32 zoneId, const std::string & bantext)
				: iSysopId(sysopId),
				  iZoneId(zoneId),
				  iBanText(bantext)
			{ }

			void operator()(Transaction & t)
			{
				std::string query = Format("select updatebantext from isometry.globalop, isometry.opgroup where id=groupid and playerid=%d", iSysopId);
				pqxx::result res = t.exec(query);

				bool updateBanText = false;
				if(!res.empty())
					res[0][0].to(updateBanText);

				if(!updateBanText)
				{
					query = Format("select updatebantext from isometry.localop, isometry.opgroup where id=groupid and playerid=%d and zoneid=%d", iSysopId, iZoneId);
					res = t.exec(query);

					if(!res.empty())
						res[0][0].to(updateBanText);

					if(!updateBanText)
						throw permission_error("not a sysop or no permission");
				}

				query = Format("update isometry.zone set bantext='%s' where id=%d", esc(iBanText).c_str(), iZoneId);
				t.exec(query);
			}

		private:
			uint32 iSysopId;
			uint32 iZoneId;
			const std::string & iBanText;
	};
}

#pragma once

#include "base.h"
#include "DbLib.h"

namespace Database
{
	class BanAddComment : public DbAction
	{
		public:
			explicit BanAddComment(uint32 sysopId, uint32 zoneId, uint32 banId, const std::string & comment)
				: iSysopId(sysopId),
				  iZoneId(zoneId),
				  iBanId(banId),
				  iComment(comment)
			{ }

			void operator()(Transaction & t)
			{
				std::string query = Format("select access, zoneid from isometry.ban where id=%d", iBanId);
				pqxx::result res = t.exec(query);

				if(res.empty())
					throw ban_not_found_error(Format("%d", iBanId));

				uint32 targetAccess;
				uint32 targetZone;
				res[0][0].to(targetAccess);
				if(!res[0][1].is_null())
					res[0][1].to(targetZone);

				bool isNetBan = res[0][1].is_null();

				query = Format("select cannetban, access from isometry.opgroup, isometry.globalop where id=groupid and playerid=%d", iSysopId);
				res = t.exec(query);

				bool canNetBan = false;
				uint32 access = ~0;
				if(!res.empty())
				{
					res[0][0].to(canNetBan);
					res[0][1].to(access);
				}

				if(access > targetAccess || (isNetBan && !canNetBan))
				{
					if(!isNetBan && iZoneId != targetZone)
						throw permission_error("not in the same zone as ban");

					query = Format("select cannetban, access from isometry.opgroup, isometry.localop where id=groupid and playerid=%d and zoneid=%d", iSysopId, iZoneId);
					res = t.exec(query);
					if(!res.empty())
					{
						res[0][0].to(canNetBan);
						res[0][1].to(access);
					}

					if(access > targetAccess || (isNetBan && !canNetBan))
						throw permission_error("cannot change ban");
				}

				query = Format("update isometry.ban set comment='%s' where id=%d", esc(iComment).c_str(), iBanId);
				t.exec(query);
			}

		private:
			uint32 iSysopId;
			uint32 iZoneId;
			uint32 iBanId;
			const std::string & iComment;
	};
}

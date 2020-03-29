#pragma once

#include "base.h"
#include "DbLib.h"

namespace Database
{
	class BanChange : public DbAction
	{
		public:
			explicit BanChange(uint32 sysopId, uint32 zoneId, uint32 banId, bool * banAllInRange, uint32 * banDayCount, uint32 * access, const InetAddress * ipAddress, uint32 * netmask, uint32 * machineId)
				: iSysopId(sysopId),
				  iZoneId(zoneId),
				  iBanId(banId),
				  iBanAllInRange(banAllInRange),
				  iBanDayCount(banDayCount),
				  iAccess(access),
				  iIpAddress(ipAddress),
				  iNetmask(netmask),
				  iMachineId(machineId)
			{ }

			void operator()(Transaction & t)
			{
				std::string query = Format("select access, zoneid from isometry.ban where id=%d", iBanId);
				pqxx::result res = t.exec(query);

				if(res.empty())
					throw ban_not_found_error(Format("%d", iBanId));

				uint32 targetAccess;
				res[0][0].to(targetAccess);
				bool isNetban = res[0][1].is_null();

				query = Format("select g.access, g.cannetban, trunc(extract(epoch from g.netbanperiod)/3600/24), g.netbanbyip, g.netipmask, g.netblockold, trunc(extract(epoch from g.banperiod)/3600/24), g.banbyip, g.ipmask, g.blockold from isometry.opgroup as g, isometry.globalop as o where o.playerid=%d and g.id=o.groupid", iSysopId);
				res = t.exec(query);

				if(!CheckPermission(res, targetAccess, isNetban))
				{
					query = Format("select g.access, g.cannetban, trunc(extract(epoch from g.netbanperiod)/3600/24), g.netbanbyip, g.netipmask, g.netblockold, trunc(extract(epoch from g.banperiod)/3600/24), g.banbyip, g.ipmask, g.blockold from isometry.opgroup as g, isometry.localop as o where o.playerid=%d and o.zoneid=%d and g.id=o.groupid", iSysopId, iZoneId);
					res = t.exec(query);

					if(!CheckPermission(res, targetAccess, isNetban))
						throw permission_error("no permission");
				}

				if(iBanDayCount == 0 && iIpAddress == 0 && iNetmask == 0 && iMachineId == 0 && iAccess == 0)
					return;

				query = "update isometry.ban set";
				if(iBanDayCount != 0)
					query += Format(" banperiod='%d days' ", *iBanDayCount);
				if(iIpAddress != 0 && iNetmask != 0)
					query += Format(" ipaddress=inet '%s/%d' ", iIpAddress->GetIpAddress().c_str(), *iNetmask);
				else if(iIpAddress != 0)
					query += Format(" ipaddress=inet ('%s/' || masklen(ipaddress)) ", iIpAddress->GetIpAddress().c_str());
				else if(iNetmask != 0)
					query += Format(" ipaddress=inet (host(ipaddress) || '/%d') ", *iNetmask);
				if(iMachineId != 0)
					query += Format(" machineid=%d ", *iMachineId);
				if(iAccess != 0)
					query += Format(" access=%d ", *iAccess);
				query += Format("where id=%d", iBanId);

				res = t.exec(query);
			}

		private:
			bool CheckPermission(const pqxx::result & res, uint32 targetAccess, bool isnetban)
			{
				if(res.empty())
					return false;

				uint32 access;
				bool cannetban;
				uint32 netbandays;
				bool netbanipoverride;
				uint32 netbanipmask;
				bool netbanblockold;

				uint32 bandays;
				bool banipoverride;
				uint32 ipmask;
				bool blockold;

				res[0][0].to(access);
				res[0][1].to(cannetban);
				res[0][2].to(netbandays);
				res[0][3].to(netbanipoverride);
				res[0][4].to(netbanipmask);
				res[0][5].to(netbanblockold);
				res[0][6].to(bandays);
				res[0][7].to(banipoverride);
				res[0][8].to(ipmask);
				res[0][9].to(blockold);

				if(access > targetAccess || (isnetban && !cannetban))
					return false;

				if(isnetban)
					return CheckPermission2(access, netbandays, netbanipoverride, netbanipmask, netbanblockold);
				return CheckPermission2(access, bandays, banipoverride, ipmask, blockold);
			}

			bool CheckPermission2(uint32 access, uint32 bandays, bool ipoverride, uint32 ipmask, bool blockold)
			{
				return ((iAccess == 0 || *iAccess >= access) &&
				        (iBanDayCount == 0 || *iBanDayCount <= bandays) &&
				        (iIpAddress == 0 || ipoverride) &&
				        (iNetmask == 0 || *iNetmask >= ipmask) &&
				        (iBanAllInRange == 0 || blockold)
				       );
			}

			uint32 iSysopId;
			uint32 iZoneId;
			uint32 iBanId;
			bool * iBanAllInRange;
			uint32 * iBanDayCount;
			uint32 * iAccess;
			const InetAddress * iIpAddress;
			uint32 * iNetmask;
			uint32 * iMachineId;
	};
}

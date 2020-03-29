#pragma once

#include "base.h"
#include "DbLib.h"
#include "Ban.h"
#include "Exceptions.h"

namespace Database
{
	class BanAdd : public DbAction
	{
		public:
			explicit BanAdd(uint32 sysopId, uint32 zoneId, uint32 playerId, const std::string & comment, bool netban, bool banAllInRange, uint32 banDayCount, uint32 access, const InetAddress & ipAddress, const InetAddress * ipOverride, uint32 netmask, uint32 machineId, const uint32 * machineIdOverride, const BanData & banData, uint32 & banId)
				: iSysopId(sysopId),
				  iZoneId(zoneId),
				  iPlayerId(playerId),
				  iComment(comment),
				  iNetBan(netban),
				  iBanAllInRange(banAllInRange),
				  iBanDayCount(banDayCount),
				  iAccess(access),
				  iIpAddress(ipAddress),
				  iIpOverride(ipOverride),
				  iNetmask(netmask),
				  iMachineId(machineId),
				  iMachineIdOverride(machineIdOverride),
				  iBanData(banData),
				  iBanId(banId)
			{ }

			void operator()(Transaction & t)
			{
				std::string query = Format("select true from isometry.globalop where playerid=%d", iPlayerId);
				pqxx::result res = t.exec(query);

				if(res.empty())
				{
					query = Format("select true from isometry.localop where playerid=%d and zoneid=%d", iPlayerId, iZoneId);
					res = t.exec(query);
				}

				if(!res.empty())
					throw operation_not_permitted_error("target is a sysop");

				query = Format("select g.access, g.cannetban, trunc(extract(epoch from g.netbanperiod)/3600/24), g.netbanbyip, g.netipmask, g.netblockold, trunc(extract(epoch from g.banperiod)/3600/24), g.banbyip, g.ipmask, g.blockold from isometry.opgroup as g, isometry.globalop as o where o.playerid=%d and g.id=o.groupid", iSysopId);
				res = t.exec(query);

				if(!CheckPermission(res))
				{
					query = Format("select g.access, g.cannetban, trunc(extract(epoch from g.netbanperiod)/3600/24), g.netbanbyip, g.netipmask, g.netblockold, trunc(extract(epoch from g.banperiod)/3600/24), g.banbyip, g.ipmask, g.blockold from isometry.opgroup as g, isometry.localop as o where o.playerid=%d and o.zoneid=%d and g.id=o.groupid", iSysopId, iZoneId);
					res = t.exec(query);

					if(!CheckPermission(res))
						throw permission_error("no permission");
				}

				const InetAddress * ip = (iIpOverride) ? iIpOverride : &iIpAddress;
				const uint32 * machineId = (iMachineIdOverride) ? iMachineIdOverride : &iMachineId;

				if(iNetBan)
					query = Format("insert into isometry.ban (banperiod, bannedby, playerid, ipaddress, machineid, access, bandata, comment) values ('%d days', %d, %d, inet '%s/%d', %d, %d, ARRAY[%s], '%s') returning id", iBanDayCount, iSysopId, iPlayerId, ip->GetIpAddress().c_str(), iNetmask, *machineId, iAccess, iBanData.AsSqlArray().c_str(), esc(iComment).c_str());
				else
					query = Format("insert into isometry.ban (banperiod, bannedby, playerid, zoneid, ipaddress, machineid, access, bandata, comment) values ('%d days', %d, %d, %d, inet '%s/%d', %d, %d, ARRAY[%s], '%s') returning id", iBanDayCount, iSysopId, iPlayerId, iZoneId, ip->GetIpAddress().c_str(), iNetmask, *machineId, iAccess, iBanData.AsSqlArray().c_str(), esc(iComment).c_str());

				res = t.exec(query);

				if(res.empty())
					throw system_error("unable to create ban entry in database");

				res[0][0].to(iBanId);
			}

		private:
			bool CheckPermission(const pqxx::result & res)
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

				if(iNetBan && !cannetban)
					return false;

				if(iNetBan)
					return CheckPermission2(access, netbandays, netbanipoverride, netbanipmask, netbanblockold);
				return CheckPermission2(access, bandays, banipoverride, ipmask, blockold);
			}

			bool CheckPermission2(uint32 access, uint32 bandays, bool ipoverride, uint32 ipmask, bool blockold)
			{
				return (iAccess >= access &&
				        iBanDayCount <= bandays &&
				        (ipoverride || iIpOverride == 0) &&
				        iNetmask >= ipmask &&
				        (blockold || !iBanAllInRange)
				       );
			}

			uint32 iSysopId;
			uint32 iZoneId;
			uint32 iPlayerId;
			const std::string & iComment;
			bool iNetBan;
			bool iBanAllInRange;
			uint32 iBanDayCount;
			uint32 iAccess;
			const InetAddress & iIpAddress;
			const InetAddress * iIpOverride;
			uint32 iNetmask;
			uint32 iMachineId;
			const uint32 * iMachineIdOverride;
			const BanData & iBanData;
			uint32 & iBanId;
	};
}

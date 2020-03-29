#pragma once

#include "base.h"
#include "DbLib.h"
#include "NetLib.h"
#include "Exceptions.h"

namespace Database
{
	class ZoneLogin : public DbAction
	{
		public:
			ZoneLogin(const std::string & name, const std::string & password, uint32 serverId, uint32 groupId, uint32 scoreId, const InetAddress & address, uint32 & id, std::string & bantext, bstring & sscid)
				: iName(name),
				  iPassword(password),
				  iServerId(serverId),
				  iGroupId(groupId),
				  iScoreId(scoreId),
				  iAddress(address),
				  iId(id),
				  iBanText(bantext),
				  iSscId(sscid),
				  iNewCreated(false)
			{ }

			void operator()(Transaction & t)
			{
				std::string query = Format("select password, name, id, bantext, now()-lastactivedate > zonereclaimtime from isometry.zone, isometry.config where isactive=true and serverid=%d and activeconfig=true", iServerId);
				pqxx::result res = t.exec(query);
				if(res.empty())
				{
					query = Format("insert into isometry.zone (name, password, serverid, groupid, scoreid, ipaddress, port) values ('%s', '%s', %d, %d, %d, inet '%s', %d) returning id, bantext", esc(iName).c_str(), esc(iPassword).c_str(), iServerId, iGroupId, iScoreId, iAddress.GetIpAddress().c_str(), iAddress.GetPort());
					res = t.exec(query);

					if(res.empty())
						throw system_error("could not create zone entry in database");

					res[0][0].to(iId);
					res[0][1].to(iBanText);
					iNewCreated = true;
				}
				else
				{
					std::string dbPassword;
					std::string dbName;
					bool canReclaim;

					res[0][0].to(dbPassword);
					res[0][1].to(dbName);
					res[0][4].to(canReclaim);

					if(!canReclaim && iPassword != dbPassword)
						throw permission_error("invalid password");

					res[0][2].to(iId);
					res[0][3].to(iBanText);

					query = Format("update isometry.zone set name='%s', password='%s', ipaddress=inet '%s', port=%d where id=%d", esc(iName).c_str(), esc(iPassword).c_str(), iAddress.GetIpAddress().c_str(), iAddress.GetPort(), iId);
					t.exec(query);
				}

				query = Format("select sscid from isometry.sscid where ipaddress=inet '%s' and port=%d", iAddress.GetIpAddress().c_str(), iAddress.GetPort());
				res = t.exec(query);

				if(!res.empty())
				{
					pqxx::binarystring s(res[0][0]);
					iSscId.Assign(s.c_ptr(), s.size());
				}
			}

			bool NewCreated() const { return iNewCreated; }

		private:
			std::string iName;
			std::string iPassword;
			uint32 iServerId;
			uint32 iGroupId;
			uint32 iScoreId;
			InetAddress iAddress;
			uint32 & iId;
			std::string & iBanText;
			bstring & iSscId;

			bool iNewCreated;
	};
}

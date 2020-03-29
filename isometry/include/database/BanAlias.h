#pragma once

#include "base.h"
#include "DbLib.h"
#include "Ban.h"
#include "Exceptions.h"

namespace Database
{
	class BanAlias : public DbAction
	{
		private:
			struct Alias
			{
				std::string iName;
				std::string iIpAddress;
				uint32 iMachineId;
				uint32 iAdditionalMatches;

				Alias() : iMachineId(0), iAdditionalMatches(0) {}
			};

			typedef std::map <std::string, Alias> container;

		public:
			typedef container::const_iterator iterator;

			explicit BanAlias(uint32 playerId, uint32 zoneId, const std::string & target)
				: iPlayerId(playerId),
				  iZoneId(zoneId),
				  iTarget(target)
			{ }

			void operator()(Transaction & t)
			{
				std::string query = Format("select g.cancheckalias from isometry.globalop as o, isometry.opgroup as g where o.playerid=%d and o.groupid=g.id", iPlayerId);
				pqxx::result res = t.exec(query);

				bool canCheckAlias = false;
				if(!res.empty())
					res[0][0].to(canCheckAlias);

				if(!canCheckAlias)
				{
					query = Format("select g.cancheckalias from isometry.localop as o, isometry.opgroup as g where o.zoneid=%d and o.playerid=%d and o.groupid=g.id", iZoneId, iPlayerId);
					res = t.exec(query);

					if(!res.empty())
						res[0][0].to(canCheckAlias);
				}

				if(!canCheckAlias)
					throw permission_error("cannot check aliases");

				query = Format("select id from isometry.player where lower(name)='%s'", esc(String::ToLowerCopy(iTarget)).c_str());
				res = t.exec(query);

				if(res.empty())
					throw not_found_error("player");

				uint32 targetId;
				res[0][0].to(targetId);

				query = Format("select machineid, ipaddress, bandata from isometry.loginhistory where playerid=%d order by logintime desc limit 5", targetId);
				res = t.exec(query);

				if(res.empty())
					throw not_found_error("player history");

				for(uint32 i = 0; i < res.size(); ++i)
				{
					uint32 machineId;
					std::string ipString;

					res[i][0].to(machineId);
					res[i][1].to(ipString);

					if(ipString != "127.0.0.1")
					{
						query = Format("select distinct p.name from isometry.player as p, isometry.loginhistory as h where h.playerid!=%d and p.id=h.playerid and h.ipaddress=inet '%s'", targetId, ipString.c_str());
						pqxx::result res2 = t.exec(query);

						for(uint32 j = 0; j < res2.size(); ++j)
						{
							std::string name;
							res2[j][0].to(name);

							Alias & alias = iAliases[name];
							alias.iName = name;
							if(alias.iIpAddress.empty())
								alias.iIpAddress = ipString;
							else
								++alias.iAdditionalMatches;
						}
					}

					query = Format("select distinct p.name from isometry.player as p, isometry.loginhistory as h where h.playerid!=%d and p.id=h.playerid and h.machineid=%d", targetId, machineId);
					pqxx::result res2 = t.exec(query);

					for(uint32 j = 0; j < res2.size(); ++j)
					{
						std::string name;
						res2[j][0].to(name);

						Alias & alias = iAliases[name];
						alias.iName = name;
						if(alias.iMachineId == 0)
							alias.iMachineId = machineId;
						else
							++alias.iAdditionalMatches;
					}
				}
			}

			iterator begin() const { return iAliases.begin(); }
			iterator end() const { return iAliases.end(); }
			bool empty() const { return iAliases.empty(); }

		private:
			uint32 iPlayerId;
			uint32 iZoneId;
			const std::string & iTarget;
			container iAliases;
	};
}

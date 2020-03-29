#pragma once

#include "base.h"
#include "DbLib.h"
#include "CryptoLib.h"
#include "Exceptions.h"
#include "Ban.h"
#include "Player.h"

namespace Database
{
	class PlayerLogin : public DbAction
	{
		public:
			PlayerLogin(const std::string & name, const std::string & password, bool createNew, uint32 zoneId, uint32 scoreId, uint32 machineId, const std::string & ipAddress, uint32 timezonebias, const BanData & banData)
				: iName(name),
				  iPassword(password),
				  iCreateNew(createNew),
				  iZoneId(zoneId),
				  iScoreId(scoreId),
				  iMachineId(machineId),
				  iIpAddress(ipAddress),
				  iTimeZoneBias(timezonebias),
				  iBanData(banData)
			{ }

			void operator()(Transaction & t)
			{
				uint32 id;

				if(!Player::IsValidName(iName))
					throw malformed_error("player name", iName);

				std::string query = Format("select password, id, now()-lastseendate > playerreclaimtime from isometry.player, isometry.config where lower(name)='%s' and activeconfig=true", esc(String::ToLowerCopy(iName)).c_str());
				pqxx::result res = t.exec(query);
				if(res.empty())
				{
					if(!iCreateNew)
						throw player_not_found_error(iName);

					query = Format("insert into isometry.player (name, password) values ('%s', '%s') returning id", esc(iName).c_str(), esc(iPassword).c_str());
					res = t.exec(query);

					if(res.empty())
						throw system_error("unable to create player record in database");

					res[0][0].to(id);
				}
				else
				{
					std::string password;
					bool canReclaim;

					res[0][0].to(password);
					res[0][1].to(id);
					res[0][2].to(canReclaim);

					query = Format("select nonce from isometry.loginnonce where ipaddress=inet '%s' and now() < expirytime", iIpAddress.c_str());
					res = t.exec(query);
					if(!res.empty())
					{
						std::string nonce;
						res[0][0].to(nonce);

						query = Format("delete from isometry.loginnonce where ipaddress=inet '%s' or expirytime < now()", iIpAddress.c_str());
						t.exec(query);

						Sha1 hasher;
						hasher << nonce << password;
						bstring hash = hasher.Hash();

						password.resize(hash.size());
						for(bstring::size_type i = 0; i < hash.size(); ++i)
							if(hash[i] == 0)
								password[i] = 0x01;
							else
								password[i] = hash[i];
					}

					if(!canReclaim && password != iPassword)
						throw credential_error("bad password");

					if(password != iPassword)
						t.exec(Format("update isometry.player set password='%s' where id=%d", esc(iPassword).c_str(), id));
				}

				//
				// Log this attempt to enter the zone - the player may or may not be banned, we don't
				// know yet but we still want to capture the information so far.
				//
				t.exec(Format("insert into isometry.loginhistory (playerid, zoneid, ipaddress, machineid, timezonebias, bandata) values (%d, %d, inet '%s', %d, %d, ARRAY[%s])", id, iZoneId, iIpAddress.c_str(), iMachineId, iTimeZoneBias, iBanData.AsSqlArray().c_str()));

				//
				// If there are any local or netwide banfrees, we'll bypass the ban checks.
				//
				res = t.exec(Format("select true from isometry.banfree where playerid=%d and (zoneid is null or zoneid=%d)", id, iZoneId));
				if(!res.empty())
					return;

				//
				// Being an operator gives you an automatic banfree.
				//
				res = t.exec(Format("select true from isometry.globalop where playerid=%d", id));
				if(!res.empty())
					return;

				res = t.exec(Format("select true from isometry.localop where playerid=%d and zoneid=%d", id, iZoneId));
				if(!res.empty())
					return;

				//
				// Check ban list
				//
				query = Format("select b.id, bantime+banperiod from isometry.ban as b, isometry.player where (zoneid=%d or zoneid is null) and now() < bantime+banperiod and (playerid=%d or machineid=%d or inet '%s' << ipaddress or bandata=ARRAY[%s])", iZoneId, id, iMachineId, iIpAddress.c_str(), iBanData.AsSqlArray().c_str());
				res = t.exec(query);
				if(!res.empty())
				{
					uint32 banId;
					std::string bannedUntil;

					res[0][0].to(banId);
					res[0][1].to(bannedUntil);

					throw banned_error(bannedUntil, banId);
				}

				//
				// Check for ban data alias
				//
				query = Format("select bandata, isometry.bandatamatch(bandata, ARRAY[%s]) as score from isometry.ban where (zoneid=%d or zoneid is null) order by score desc limit 1", iBanData.AsSqlArray().c_str(), iZoneId);
				res = t.exec(query);
				if(!res.empty())
				{
					uint32 score;
					res[0][1].to(score);
					if(score >= 12)
						throw banned_error("the end of time", -1);
				}

				//
				// Check for email alias
				//
				query = Format("select true from isometry.player as p1, isometry.player as p2, isometry.ban as b where p1.id=%d and p1.email=p2.email and p2.id=b.playerid and (b.zoneid=%d or zoneid is null)", id, iZoneId);
				res = t.exec(query);
				if(!res.empty())
					throw banned_error("the end of time", -2);
			}

		private:
			std::string iName;
			std::string iPassword;
			bool iCreateNew;
			uint32 iZoneId;
			uint32 iScoreId;
			uint32 iMachineId;
			const std::string & iIpAddress;
			uint32 iTimeZoneBias;
			const BanData & iBanData;
	};
}

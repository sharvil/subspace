#pragma once

#include "base.h"
#include "NetLib.h"
#include "TimeLib.h"
#include "Ban.h"

#include <vector>

class SSConnection;
class Zone;
class Chat;
class Squad;

class Player
{
	public:
		enum EKickoffReason
		{
			KKickBanned
		};

		struct Score
		{
			uint32 iWins;
			uint32 iLosses;
			uint32 iGoals;
			uint32 iPoints;
			uint32 iFlagPoints;
		};

		explicit Player(Zone & zone, const std::string & name, uint32 connectionId, uint32 scoreId, const InetAddress & ipAddress, uint32 machineId, const BanData & bandata);
		~Player();

		static bool IsValidName(const std::string & name);

		void Poll(SSConnection & connection);
		void SendMessage(const std::string & message);
		void SendMessage(const std::string & prefix, const StringSequence & strings, std::string::value_type join, std::string::size_type maxLength = 80);
		void SendChatMessage(const Chat & chat, const std::string & message);
		void SendRemotePlayerMessage(const std::string & message);
		void SendRawPacket(const bstring & packet);
		void Kickoff(const EKickoffReason & reason);

		const Zone & GetZone() const;
		uint32 GetId() const;
		uint32 GetConnectionId() const;
		uint32 GetScoreId() const;
		const std::string & GetName() const;
		const std::string & GetSquadName() const;
		const bstring & GetBanner() const;
		uint32 GetSecondsPlayed() const;
		const std::string & GetCreationDate() const;
		const Score & GetScore() const;
		InetAddress GetIpAddress() const;
		uint32 GetMachineId() const;
		const BanData & GetBanData() const;

		void UpdateBanner(const bstring & banner);
		void UpdateScore(const Score & score);

		void ChatKicked(const Chat & chat);
		void ChatDeleted(const Chat & chat);

		void SquadKicked();
		void SquadDeleted();

		void BanKicked(bool kickoff);

		void ProcessCommand(const std::string & message);
		void ProcessChatMessage(uint32 ordinal, const std::string & message);

	private:
		typedef std::vector <Chat *> Channels;

		void ProcessBillerMessage(const std::string & message);

		void ProcessChat();
		void ProcessChatCreate(const std::string & chat);
		void ProcessChatDelete(const std::string & chat);
		void ProcessChatGrant(const std::string & chat, const std::string & player);
		void ProcessChatJoin(uint32 ordinal, const std::string & chat);
		void ProcessChatLeave(const std::string & chat);
		void ProcessChatAllow(const std::string & chat, const std::string & player);
		void ProcessChatDeny(const std::string & chat, const std::string & player);
		void ProcessChatMode(const std::string & chat, const std::string & mode);
		void ProcessChatJoinList(const std::string & chatlist);
		void ProcessChatInfo(const std::string & chat);
		void ProcessChatFind(const std::string & player);
		void ProcessChatList();

		void ProcessSquad();
		void ProcessSquadCreate(const std::string & name);
		void ProcessSquadDelete(const std::string & name);
		void ProcessSquadGrant(const std::string & name);
		void ProcessSquadJoin(const std::string & name);
		void ProcessSquadLeave();
		void ProcessSquadAllow(const std::string & name);
		void ProcessSquadDeny(const std::string & name);
		void ProcessSquadMode(const std::string & mode);
		void ProcessSquadOwner(const std::string & squad);
		void ProcessSquadList();
		void ProcessSquadListMembers(const std::string & name);

		void ProcessAddOp(uint32 access, bool netOp, const std::string & name);
		void ProcessRemoveOp(const std::string & name, bool netOp);
		void ProcessListOp(bool netwide);
		void ProcessBan(const std::string & name, const std::string & comment, bool netBan, bool banAllInRange, bool removeIdNumber, bool dontKick, bool banInPlayerZone, uint32 banDayCount, uint32 access, const InetAddress & ipAddress, uint32 ipMask, uint32 machineId);
		void ProcessChangeBan(uint32 banId, bool * banAllInRange, bool * removeIdNumber, uint32 * banDayCount, uint32 * access, const InetAddress * ipAddress, uint32 * ipMask, uint32 * machineId);
		void ProcessLiftBan(uint32 banId);
		void ProcessListBan(bool netban, bool details, uint32 limit);
		void ProcessBanComment(uint32 banId, const std::string & name);
		void ProcessBanFree(const std::string & name, uint32 access, bool netban);
		void ProcessRemoveBanFree(const std::string & name, bool netban);
		void ProcessListBanFree(bool netban, uint32 limit);

		void ProcessTop10Points();
		void ProcessTop10Kills();
		void ProcessTop10Deaths();
		void ProcessTop10SquadPoints();
		void ProcessTop10SquadKills();
		void ProcessTop10SquadDeaths();

		void ProcessMessage(const std::string & player, const std::string & message);
		void ProcessMessages();

		void ProcessSetPassword(const std::string & newPassword);

		void ProcessAlias(const std::string & name);

		void ProcessZone();
		void ProcessZones();

		void ProcessFind(const std::string & name);

		void ProcessVersion();

		Zone & iZone;
		uint32 iId;
		uint32 iConnectionId;
		uint32 iScoreId;
		std::string iName;
		InetAddress iIpAddress;
		uint32 iMachineId;
		BanData iBanData;
		bstring iBanner;
		uint32 iSecondsPlayed;
		std::string iCreationDate;
		Channels iChannels;
		Squad * iSquad;
		Score iScore;

		bool iReadMessages;
		Time iSessionStartTime;
};

inline bool operator == (const std::string & name, const Player & player)
{
	return (String::Compare(name, player.GetName(), String::KCaseInsensitive) == 0);
}

inline bool operator != (const std::string & name, const Player & player)
{
	return !(name == player);
}

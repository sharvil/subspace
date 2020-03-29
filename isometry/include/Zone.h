#pragma once

#include "base.h"
#include "SSNetLib.h"
#include "Player.h"
#include "DbLib.h"
#include "CpuLib.h"

#include <map>
#include <list>

class Logger;

class Zone : public ISSProtocol
{
	private:
		enum
		{
			S2B_Ping = 0x01,
			S2B_ServerConnect = 0x02,
			S2B_ServerDisconnect = 0x03,
			S2B_UserLogin = 0x04,
			S2B_UserLogout = 0x05,
			S2B_RemoteZoneMessage = 0x06,
			S2B_UserRemotePrivateChat = 0x07,
			// 0x08 = Request File
			// 0x09 = Invalid Packet
			// 0x0A = Invalid Packet
			// 0x0B = Invalid Packet
			// 0x0C = Invalid Packet
			S2B_UserDemographics = 0x0D,
			S2B_UserPrivateChat = 0x0E,
			// 0x0F = File Transfer
			S2B_UserBanner = 0x10,
			S2B_UserScore = 0x11,
			// 0x12 = Send Message to Zones in same ScoreID
			S2B_UserCommand = 0x13,
			S2B_UserChannelChat = 0x14,
			S2B_ServerCapabilities = 0x15
		};

		enum LoginStatus
		{
			KLoginOk,
			KLoginNewUser,
			KLoginInvalidPassword,
			KLoginBanned,
			KLoginNoNewConnections,
			KLoginBadUsername,
			KLoginDemoVersion,
			KLoginServerBusy,
			KLoginAskDemographics
		};

		enum ServerCapabilityFlags
		{
			KMulticastChat        = 0x01,
			KSupportsDemographics = 0x02
		};

	public:
		explicit Zone(SSConnection & connection);
		~Zone();

		void SendMessage(const std::string & message);
		void Send(const bstring & packet);
		void Disconnect();

		uint32 GetId() const;
		uint32 GetServerId() const;
		uint32 GetScoreId() const;
		const std::string & GetName() const;
		const std::string & GetBanText() const;
		uint32 GetPlayerCount() const;

		void UpdateBanText(const std::string & bantext);

		void OnConnect(SSConnection & connection);
		void OnReceive(SSConnection & connection, const bstring & packet);
		void Poll(SSConnection & connection);
		void OnDisconnect();

	private:
		typedef DataReader <LittleEndian, Ascii> reader_t;
		typedef DataWriter <LittleEndian, AsciiZ> writer_t;
		typedef std::map <uint32, Player *> PlayerMap;
		typedef std::list <bstring> WriteQueue;

		static const uint32 KConnectionTimeoutSeconds = 10;
		static const uint32 KNoDataSeconds = 60 * 5;

		enum
		{
			KIdle,
			KAuthenticated,
			KDisconnected
		} iState;

		void ProcessServerConnect(SSConnection & connection, const bstring & packet);
		void ProcessServerDisconnect(SSConnection & connection, const bstring & packet);
		void ProcessUserLogin(SSConnection & connection, const bstring & packet);
		void ProcessUserLogout(SSConnection & connection, const bstring & packet);
		void ProcessRemoteZoneMessage(SSConnection & connection, const bstring & packet);
		void ProcessUserRemotePrivateChat(SSConnection & connection, const bstring & packet);
		void ProcessUserDemographics(SSConnection & connection, const bstring & packet);
		void ProcessUserPrivateChat(SSConnection & connection, const bstring & packet);
		void ProcessUserBanner(SSConnection & connection, const bstring & packet);
		void ProcessUserScore(SSConnection & connection, const bstring & packet);
		void ProcessUserCommand(SSConnection & connection, const bstring & packet);
		void ProcessUserChannelChat(SSConnection & connection, const bstring & packet);
		void ProcessServerCapabilities(SSConnection & connection, const bstring & packet);

		Logger & iLogger;
		SSConnection & iConnection;
		uint32 iId;
		std::string iName;
		std::string iPassword;
		uint32 iServerId;
		uint32 iScoreId;
		std::string iBanText;
		uint32 iLastPacketTime;
		PlayerMap iPlayers;
		bool iCanMulticastChat;
		bool iSupportsDemographics;

		WriteQueue iWriteQueue;
		bool iDisconnectFlag;

		CpuX86::Process iContinuumProcess;
};

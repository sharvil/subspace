#pragma once

#include "common.h"
#include "IProtocol.h"

#include <list>

class SubSpaceProtocol : public IProtocol, public NonCopyable
{
	public:
		SubSpaceProtocol();
		SubSpaceProtocol(const std::string & username, const std::string & password);
		~SubSpaceProtocol();

		bool IsConnected() const;
		bool IsLoggedIn() const;
		bool IsInArena() const;

		virtual void GetArenaList();
		virtual void ChangeShip(uint8 ship);
		virtual void EnterArena(const std::string & arenaName = std::string());
		virtual void SendChatMessage(plid_t to, const EChatType & type, const std::string & message);
		virtual void SendRegistrationForm(std::string realName, std::string emailAddress);

	protected:
		virtual void OnPoll() {}
		virtual void OnLoginResponse(const ELoginResponse & response, bool requestRegistration) {}
		virtual void OnLoginMessage(const std::string & message) {}
		virtual void OnArenaLogin() {}
		virtual void OnChatMessage(plid_t from, const EChatType & type, const std::string & message) {}
		virtual void OnPlayerEntering(const Player & player) {}
		virtual void OnPlayerLeaving(plid_t player) {}
		virtual void OnPlayerDied(plid_t player, plid_t killer) {}
		virtual void OnArenaList(const ArenaList & list) {}
		virtual void OnShipTeamChange(plid_t player, uint8 ship, uint16 team) {}
		virtual void OnLogout() {}

		plid_t iUid;
		std::string iUsername;
		std::string iPassword;
		bool iNewUser;

	private:
		typedef DataReader <LittleEndian, AsciiZ> reader_t;
		typedef DataWriter <LittleEndian, Ascii> writer_t;
		typedef std::list <std::pair <bstring, bool> > WriteQueue;

		void Poll(SSConnection & connection);
		void OnConnect(SSConnection & connection);
		void OnReceive(SSConnection & connection, const bstring & packet);
		void OnDisconnect();

		//
		// Packet handlers
		//
		void ProcessOracleResponse(SSConnection & connection, const bstring & packet);
		void ProcessUidAssignment(SSConnection & connection, const bstring & packet);
		void ProcessArenaLoginComplete(SSConnection & connection, const bstring & packet);
		void ProcessPlayerLogin(SSConnection & connection, const bstring & packet);
		void ProcessPlayerLogout(SSConnection & connection, const bstring & packet);
		void ProcessPlayerDied(SSConnection & connection,const bstring & packet);
		void ProcessChatMessage(SSConnection & connection, const bstring & packet);
		void ProcessLoginResponse(SSConnection & connection, const bstring & packet);
		void ProcessArenaSettings(SSConnection & connection, const bstring & packet);
		void ProcessFileTransfer(SSConnection & connection, const bstring & packet);
		void ProcessSynchronizeRequest(SSConnection & connection, const bstring & packet);
		void ProcessFileRequest(SSConnection & connection, const bstring & packet);
		void ProcessShipTeamChange(SSConnection & connection, const bstring & packet);
		void ProcessMapMetadata(SSConnection & connection, const bstring & packet);
		void ProcessMapData(SSConnection & connection, const bstring & packet);
		void ProcessArenaListing(SSConnection & connection, const bstring & packet);
		void ProcessLoginComplete(SSConnection & connection, const bstring & packet);
		void ProcessLoginMessage(SSConnection & connection, const bstring & packet);

		bstring Uncompress(const bstring & data, bstring::size_type offset = 0) const;
		bstring Uncompress(const uint8 * data, bstring::size_type length) const;

		void LoadMap(const bstring & data);

		uint32 GenerateMapChecksum(uint32 key) const;
		uint32 GenerateSettingsChecksum(uint32 key) const;

		static const std::string KMapPath;
		static const std::string KFilePath;

		//
		// We need these to generate checksums.
		//
		uint8 * iMapData;
		uint32 * iSettings;

		uint32 iLastPositionUpdate;
		uint32 iLastSecurityKey;
		WriteQueue iWriteQueue;

		bool iIsConnected;
		bool iIsLoggedIn;
		bool iInArena;
};

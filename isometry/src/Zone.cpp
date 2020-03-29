#include "base.h"
#include "Zone.h"
#include "Database.h"
#include "Player.h"
#include "Squad.h"
#include "ZoneManager.h"
#include "PlayerManager.h"
#include "SquadManager.h"
#include "Ban.h"

#include "CryptoLib.h"
#include "LogLib.h"
#include "TimeLib.h"

Zone::Zone(SSConnection & connection)
	: iState(KIdle),
	  iLogger(Logger::Instance()),
	  iConnection(connection),
	  iCanMulticastChat(false),
	  iSupportsDemographics(false),
	  iDisconnectFlag(false)
{
	CpuX86::PeLoader loader;
	loader.Load("Continuum40.exe", iContinuumProcess);
}

Zone::~Zone()
{
	delete &iConnection;

	for(PlayerMap::iterator i = iPlayers.begin(); i != iPlayers.end(); ++i)
		delete i->second;
}

void Zone::SendMessage(const std::string & message)
{
	DataWriter <LittleEndian, AsciiZ> writer;

	writer.AppendUint8(0x03);
	writer.AppendUint32(0x00); // This is supposed to be the server id from which the message was sent
	writer.AppendUint8(0x02);
	writer.AppendUint8(0x00);
	writer.AppendString(message);

	Send(writer.Asbstring());
}

void Zone::Send(const bstring & packet)
{
	iWriteQueue.push_back(packet);
}

void Zone::Disconnect()
{
	iDisconnectFlag = true;
}

uint32 Zone::GetId() const
{
	return iId;
}

uint32 Zone::GetServerId() const
{
	return iServerId;
}

uint32 Zone::GetScoreId() const
{
	return iScoreId;
}

const std::string & Zone::GetName() const
{
	return iName;
}

const std::string & Zone::GetBanText() const
{
	return iBanText;
}

uint32 Zone::GetPlayerCount() const
{
	return iPlayers.size();
}

void Zone::OnConnect(SSConnection & connection)
{
	iLastPacketTime = Time::GetMilliCount();
}

void Zone::OnReceive(SSConnection & connection, const bstring & packet)
{
	if(iState == KIdle && (packet.GetLength() != 173 || packet[0] != S2B_ServerConnect))
	{
		iLogger.Log(KLogWarning, "[%s] malformed zone connect request.", connection.GetRemoteAddress().AsString().c_str());
		connection.Disconnect();
		return;
	}

	if(iState == KDisconnected)
	{
		iLogger.Log(KLogWarning, "[%s] received packet from disconnected zone.", iName.c_str());
		return;
	}

	if(packet.empty())
	{
		iLogger.Log(KLogWarning, "[%s] empty packet received.", iName.c_str());
		return;
	}

	iLastPacketTime = Time::GetMilliCount();

	try
	{
		switch(packet[0])
		{
			case S2B_Ping: break;
			case S2B_ServerConnect: ProcessServerConnect(connection, packet); break;
			case S2B_ServerDisconnect: ProcessServerDisconnect(connection, packet); break;
			case S2B_UserLogin: ProcessUserLogin(connection, packet); break;
			case S2B_UserLogout: ProcessUserLogout(connection, packet); break;
			case S2B_RemoteZoneMessage: ProcessRemoteZoneMessage(connection, packet); break;
			case S2B_UserRemotePrivateChat: ProcessUserRemotePrivateChat(connection, packet); break;
			case S2B_UserDemographics: ProcessUserDemographics(connection, packet);
			case S2B_UserPrivateChat: ProcessUserPrivateChat(connection, packet); break;
			case S2B_UserBanner: ProcessUserBanner(connection, packet); break;
			case S2B_UserScore: ProcessUserScore(connection, packet); break;
			case S2B_UserCommand: ProcessUserCommand(connection, packet); break;
			case S2B_UserChannelChat: ProcessUserChannelChat(connection, packet); break;
			case S2B_ServerCapabilities: ProcessServerCapabilities(connection, packet); break;

			default:
				printf("Unhandled packet:\n%s\n", packet.AsString().c_str());
				break;
		}
	}
	catch(const std::runtime_error & e)
	{
		iLogger.Log(KLogError, "%s\n%s", e.what(), packet.AsString().c_str());
	}
	catch(const std::runtime_error & e)
	{
		iLogger.Log(KLogError, "%s\n%s", e.what(), packet.AsString().c_str());
	}
}

void Zone::Poll(SSConnection & connection)
{
	if(iDisconnectFlag)
	{
		iLogger.Log(KLogInfo, "[%s] disconnected: received disconnect signal.", connection.GetRemoteAddress().AsString().c_str());
		connection.Disconnect();
		return;
	}

	uint32 now = Time::GetMilliCount();
	if(iState == KIdle && now - iLastPacketTime > KConnectionTimeoutSeconds * 1000)
	{
		iLogger.Log(KLogInfo, "[%s] disconnected: connection timeout.", connection.GetRemoteAddress().AsString().c_str());
		connection.Disconnect();
		return;
	}

	if(now - iLastPacketTime > KNoDataSeconds * 1000)
	{
		iLogger.Log(KLogInfo, "[%s] disconnected: no data from zone.", iName.c_str());
		connection.Disconnect();
		return;
	}

	for(WriteQueue::iterator i = iWriteQueue.begin(); i != iWriteQueue.end(); ++i)
		connection.Send(*i, true);

	iWriteQueue.clear();

	for(PlayerMap::iterator i = iPlayers.begin(); i != iPlayers.end(); ++i)
		i->second->Poll(connection);
}

void Zone::OnDisconnect()
{
	if(iState == KAuthenticated)
	{
		ZoneManager::Instance().Unregister(*this);
		Database::ZoneLogout(GetId()).Execute();
	}

	iState = KDisconnected;
}

void Zone::UpdateBanText(const std::string & bantext)
{
	if(!bantext.empty())
		iBanText = bantext;
}

void Zone::ProcessServerConnect(SSConnection & connection, const bstring & packet)
{
	reader_t reader(packet);

	reader.ReadUint8();
	uint32 serverId = reader.ReadUint32();
	uint32 groupId = reader.ReadUint32();
	uint32 scoreId = reader.ReadUint32();
	std::string name = String::TrimCopy(reader.ReadString(126));
	uint16 port = reader.ReadUint16();
	std::string password = String::TrimCopy(reader.ReadString(32));
	InetAddress address(connection.GetRemoteAddress().AsUint32HostOrder(), port);

	if(name.empty())
	{
		connection.Disconnect();
		iLogger.Log(KLogWarning, "[%s] no zone name specified.", connection.GetRemoteAddress().AsString().c_str());
		return;
	}

	if(password.empty())
	{
		connection.Disconnect();
		iLogger.Log(KLogWarning, "[%s]{%s} no password specified.", name.c_str(), connection.GetRemoteAddress().AsString().c_str());
		return;
	}

	try
	{
		bstring sscid;
		Database::ZoneLogin login(name, password, serverId, groupId, scoreId, address, iId, iBanText, sscid);

		login.Execute();

		if(login.NewCreated())
			iLogger.Log(KLogInfo, "[%s]{%s} joined the network.", name.c_str(), connection.GetRemoteAddress().AsString().c_str());
		else
			iLogger.Log(KLogInfo, "[%s]{%s} re-connected successfully.", name.c_str(), connection.GetRemoteAddress().AsString().c_str());

		iState = KAuthenticated;
		iServerId = serverId;
		iName = name;
		iPassword = password;
		iScoreId = scoreId;

		if(!sscid.IsEmpty())
		{
			DataWriter <LittleEndian> writer;
			writer.AppendUint8(0x33);
			writer.AppendString(sscid);

			connection.Send(writer.Asbstring(), true);
		}

		//
		// This has to happen *after* all of the instance variables have been
		// assigned so that all of the getters will return the correct values.
		//
		ZoneManager::Instance().Register(*this);
	}
	catch(const permission_error &)
	{
		iLogger.Log(KLogWarning, "[%s]{%s} incorrect zone password.", name.c_str(), connection.GetRemoteAddress().AsString().c_str());
		connection.Disconnect();
	}
}

void Zone::ProcessServerDisconnect(SSConnection & connection, const bstring & packet)
{
	iLogger.Log(KLogInfo, "[%s] received disconnect message.", connection.GetRemoteAddress().AsString().c_str());
	connection.Disconnect();
}

void Zone::ProcessUserLogin(SSConnection & connection, const bstring & packet)
{
	reader_t reader(packet, 1);

	bool makeNew = reader.ReadUint8() != 0;
	uint32 ip = reader.ReadUint8() << 24;
	ip |= reader.ReadUint8() << 16;
	ip |= reader.ReadUint8() <<  8;
	ip |= reader.ReadUint8() <<  0;
	InetAddress ipAddress(ip);
	std::string name = String::TrimCopy(reader.ReadString(32));
	std::string password = String::TrimCopy(reader.ReadString(32));
	uint32 connectionId = reader.ReadUint32();
	uint32 machineId = reader.ReadUint32();
	uint32 timezone = reader.ReadUint32();
	uint8 unused = reader.ReadUint8();
	bool sysop = reader.ReadUint8() != 0;
	uint16 clientVersion = reader.ReadUint16();
	BanData banData;

	UNUSED_VARIABLE(unused);
	UNUSED_VARIABLE(sysop);
	UNUSED_VARIABLE(clientVersion);

	name = name.substr(0, 24);

	try
	{
		bstring clientData = reader.Readbstring(64);

		size_type offset = 0;
		uint32 key = LittleEndian::GetUint32(clientData, offset);

		CpuX86::Emulator emulator(iContinuumProcess);

		//
		// Set up the stack
		//
		uint32 stackValues[2] = { 0xABCDEF00, key };
		iContinuumProcess.iEsp = 0x12FFC4 - 8;
		iContinuumProcess.iAddressSpace.Write(iContinuumProcess.iEsp, &stackValues, sizeof(stackValues));

		//
		// Set up the keystream pointer and clear out the memory
		//
		uint32 keystreamAddress = 0x50000004;
		iContinuumProcess.iEcx = 0x50000000;
		iContinuumProcess.iAddressSpace.Write(iContinuumProcess.iEcx, &keystreamAddress, 4);
		iContinuumProcess.iAddressSpace.Fill(keystreamAddress, 0, 80);

		//
		// Run the emulator to get the expanded key
		//
		iContinuumProcess.iEip = 0x457D60;
		emulator.Run();

		//
		// Extract the expanded key
		//
		bstring expandedKey(80);
		iContinuumProcess.iAddressSpace.Read(keystreamAddress, &expandedKey[0], 80);

		//
		// Decrypt the packet data
		//
		ContinuumCrypt crypt(expandedKey);
		crypt.Decrypt(&clientData[4], 60);

		uint32 checksum = ~LittleEndian::GetUint32(clientData, offset) ^ key;
		LittleEndian::SetUint32(clientData, 4, 0);

		Crc32 crc;
		uint32 checksum$ = crc.Checksum(clientData);

		if(checksum != checksum$)
			iLogger.Log(KLogWarning, "[%s]{%s} logging in with invalid ban data.", name.c_str(), ipAddress.AsString().c_str());
		else
			for(uint32 i = 0; i < 14; ++i)
				banData.iValues[i] = LittleEndian::GetUint32(clientData, offset);
	}
	catch(...)
	{
	}

	LoginStatus status = (makeNew && iSupportsDemographics) ? KLoginAskDemographics : KLoginOk;
	try
	{
		Database::PlayerLogin(name, password, makeNew, iId, iScoreId, machineId, ipAddress.GetIpAddress(), timezone, banData).Execute();
	}
	catch(const malformed_error &)
	{
		status = KLoginBadUsername;
	}
	catch(const player_not_found_error &)
	{
		status = KLoginNewUser;
	}
	catch(const credential_error &)
	{
		status = KLoginInvalidPassword;
	}
	catch(const banned_error & error)
	{
		std::string banString = Format("You are banned from %s until %s BanId #%d.\n%s", GetName().c_str(), error.iBannedUntil.c_str(), error.iBanId, iBanText.c_str());

		DataWriter <LittleEndian, AsciiZ> writer;
		writer.AppendUint8(0x32);
		writer.AppendUint32(connectionId);
		writer.AppendUint8(0x33);
		writer.AppendString(banString);
		connection.Send(writer.Asbstring(), true);

		status = KLoginBanned;
	}
	catch(const std::exception & e)
	{
		printf("No new connections: %s\n", e.what());
		status = KLoginNoNewConnections;
	}

	DataWriter <LittleEndian, Ascii> writer;
	writer.AppendUint8(0x01);
	writer.AppendUint8(status);
	writer.AppendUint32(connectionId);
	if(status == KLoginOk || status == KLoginAskDemographics)
	{
		Player * player = iPlayers.insert(std::make_pair(connectionId, new Player(*this, name, connectionId, iScoreId, ipAddress, machineId, banData))).first->second;

		uint32 year, month, day, hour, minute, second;
		sscanf(player->GetCreationDate().c_str(), "%d-%d-%d %d:%d:%d", &year, &month, &day, &hour, &minute, &second);

		Player::Score score = player->GetScore();
		std::string squad = player->GetSquadName();
		bstring banner = player->GetBanner();

		name.resize(24);
		squad.resize(24);
		banner.Resize(96);

		writer.AppendString(name);
		writer.AppendString(squad);
		writer.AppendString(banner);
		writer.AppendUint32(player->GetSecondsPlayed());
		writer.AppendUint16(year);
		writer.AppendUint16(month);
		writer.AppendUint16(day);
		writer.AppendUint16(hour);
		writer.AppendUint16(minute);
		writer.AppendUint16(second);
		writer.AppendUint32(0);
		writer.AppendUint32(player->GetId());
		writer.AppendUint32(0);
		writer.AppendUint16(score.iWins);
		writer.AppendUint16(score.iLosses);
		writer.AppendUint16(score.iGoals);
		writer.AppendUint32(score.iPoints);
		writer.AppendUint32(score.iFlagPoints);
	}
	else
	{
		writer.AppendString(bstring(24));       // name
		writer.AppendString(bstring(24));       // squad
		writer.AppendString(bstring(96));       // banner
		writer.AppendUint32(0);
		writer.AppendUint16(0);
		writer.AppendUint16(0);
		writer.AppendUint16(0);
		writer.AppendUint16(0);
		writer.AppendUint16(0);
		writer.AppendUint16(0);
		writer.AppendUint32(0);
		writer.AppendUint32(0);
		writer.AppendUint32(0);
	}

	connection.Send(writer.Asbstring(), true);
}

void Zone::ProcessUserLogout(SSConnection & connection, const bstring & packet)
{
	reader_t reader(packet, 1);

	uint32 connectionId = reader.ReadUint32();
	uint16 disconnectReason = reader.ReadUint16();
	uint16 latency = reader.ReadUint16();
	uint16 ping = reader.ReadUint16();
	uint16 plossS2C = reader.ReadUint16();
	uint16 plossC2S = reader.ReadUint16();

	UNUSED_VARIABLE(disconnectReason);
	UNUSED_VARIABLE(latency);
	UNUSED_VARIABLE(ping);
	UNUSED_VARIABLE(plossS2C);
	UNUSED_VARIABLE(plossC2S);

	PlayerMap::iterator i = iPlayers.find(connectionId);
	if(i == iPlayers.end())
	{
		// It's okay if we can't find the player - this can happen in
		// case we return a login error. We only put authenticated users in
		// the player map but subgame does a connect/disconnect pair for
		// everyone.
		return;
	}

	if(!reader.IsEndOfData())
	{
		Player::Score score;
		score.iWins = reader.ReadUint16();
		score.iLosses = reader.ReadUint16();
		score.iGoals = reader.ReadUint16();
		score.iPoints = reader.ReadUint32();
		score.iFlagPoints = reader.ReadUint32();

		i->second->UpdateScore(score);
	}

	delete i->second;
	iPlayers.erase(i);
}

void Zone::ProcessRemoteZoneMessage(SSConnection & connection, const bstring & packet)
{
	reader_t reader(packet, 1);

	uint32 connectionId = reader.ReadUint32();
	uint32 remoteZoneId = reader.ReadUint32();
	uint8 type = reader.ReadUint8();
	uint8 sound = reader.ReadUint8();
	std::string message = reader.ReadString();

	UNUSED_VARIABLE(remoteZoneId);
	UNUSED_VARIABLE(type);
	UNUSED_VARIABLE(sound);
	UNUSED_VARIABLE(message);

	//
	// TODO: Need a way to send a message to a zone using 03 packet and then
	// use that function from here.
	//
	PlayerMap::iterator i = iPlayers.find(connectionId);
	if(i == iPlayers.end())
		iLogger.Log(KLogWarning, "[%s] connection %d not found on remote zone message.", iName.c_str(), connectionId);
	else
		;
}

void Zone::ProcessUserRemotePrivateChat(SSConnection & connection, const bstring & packet)
{
	reader_t reader(packet, 1);

	uint32 connectionId = reader.ReadUint32();
	uint32 groupId = reader.ReadUint32();
	uint8 type = reader.ReadUint8();
	uint8 sound = reader.ReadUint8();
	std::string message = reader.ReadString();

	UNUSED_VARIABLE(connectionId);
	UNUSED_VARIABLE(groupId);
	UNUSED_VARIABLE(type);
	UNUSED_VARIABLE(sound);

	//
	// The connectionId field sent by subgame always gives us -1 so we
	// can't reply to the requesting client. This behaviour has not been
	// checked against ASSS.
	//
	int8 buffer[256] = { 0 };

	if(sscanf(message.c_str(), ":%[^:]:", buffer) != 1)
		return;

	//
	// Sadly, if we strip leading and trailing whitespace from this field,
	// it's not good enough because subgame will only forward packets if
	// there's a valid match. Let's not waste bandwidth and CPU time finding
	// records and sending packets if it's going to be dropped anyhow.
	//
	std::string target = buffer;

	if(target.empty())
		return;

	try
	{
		if(target[0] == '#')
		{
			if(target.length() < 2)
				return;

			Squad * squad = SquadManager::Instance().GetByName(target.substr(1));
			squad->SendMessage(message);
		}
		else
		{
			Player * player = PlayerManager::Instance().GetByName(buffer);
			player->SendRemotePlayerMessage(message);
		}
	}
	catch(const not_found_error &)
	{ }
}

void Zone::ProcessUserDemographics(SSConnection & connection, const bstring & packet)
{
	reader_t reader(packet, 1);

	uint32 connectionId = reader.ReadUint32();
	std::string realName = reader.ReadString(32);
	std::string emailAddress = reader.ReadString(64);
	std::string city = reader.ReadString(32);
	std::string state = reader.ReadString(24);
	int8 sex = reader.ReadInt8();
	bool fromHome = reader.ReadUint8() != 0;
	bool fromWork = reader.ReadUint8() != 0;
	bool fromSchool = reader.ReadUint8() != 0;
	uint32 processorType = reader.ReadUint32();
	uint32 unknown = reader.ReadUint32();
	std::string registryRealName = reader.ReadString(40);
	std::string registryOrganization = reader.ReadString(40);

	UNUSED_VARIABLE(realName);
	UNUSED_VARIABLE(city);
	UNUSED_VARIABLE(state);
	UNUSED_VARIABLE(sex);
	UNUSED_VARIABLE(fromHome);
	UNUSED_VARIABLE(fromWork);
	UNUSED_VARIABLE(fromSchool);
	UNUSED_VARIABLE(processorType);
	UNUSED_VARIABLE(unknown);
	UNUSED_VARIABLE(registryRealName);
	UNUSED_VARIABLE(registryOrganization);

	//
	// Another 13 binary fields, each 40 bytes long - hardware driver identifiers.
	//

	PlayerMap::iterator i = iPlayers.find(connectionId);
	if(i == iPlayers.end())
		iLogger.Log(KLogWarning, "[%s] connection %d not found on user demographics.", iName.c_str(), connectionId);
	else
		Database::PlayerUpdateDemographics(i->second->GetId(), emailAddress).Execute();
}

void Zone::ProcessUserPrivateChat(SSConnection & connection, const bstring & packet)
{
	reader_t reader(packet, 1);

	uint32 sourceId = reader.ReadUint32();
	uint32 destId = reader.ReadUint32();
	std::string message = reader.ReadString();

	UNUSED_VARIABLE(sourceId);
	UNUSED_VARIABLE(destId);

	PlayerMap::iterator src = iPlayers.find(sourceId);
	PlayerMap::iterator dest = iPlayers.find(destId);

	std::string str = Format("{%s -> %s}", (src != iPlayers.end()) ? src->second->GetName().c_str() : "", (dest != iPlayers.end()) ? dest->second->GetName().c_str() : "");

	printf("Private Message:%s %s\n", str.c_str(), message.c_str());
}

void Zone::ProcessUserBanner(SSConnection & connection, const bstring & packet)
{
	reader_t reader(packet, 1);

	uint32 connectionId = reader.ReadUint32();
	bstring banner = reader.Readbstring(96);

	PlayerMap::iterator i = iPlayers.find(connectionId);
	if(i == iPlayers.end())
		iLogger.Log(KLogWarning, "[%s] connection %d not found on banner update.", iName.c_str(), connectionId);
	else
		i->second->UpdateBanner(banner);
}

void Zone::ProcessUserScore(SSConnection & connection, const bstring & packet)
{
	reader_t reader(packet, 1);

	uint32 connectionId = reader.ReadUint32();

	Player::Score score;
	score.iWins = reader.ReadUint16();
	score.iLosses = reader.ReadUint16();
	score.iGoals = reader.ReadUint16();
	score.iPoints = reader.ReadUint32();
	score.iFlagPoints = reader.ReadUint32();

	PlayerMap::iterator i = iPlayers.find(connectionId);
	if(i == iPlayers.end())
		iLogger.Log(KLogWarning, "[%s] connection %d not found on score update.", iName.c_str(), connectionId);
	else
		i->second->UpdateScore(score);
}

void Zone::ProcessUserCommand(SSConnection & connection, const bstring & packet)
{
	reader_t reader(packet, 1);

	uint32 connectionId = reader.ReadUint32();
	std::string message = reader.ReadString();

	PlayerMap::iterator i = iPlayers.find(connectionId);
	if(i == iPlayers.end())
		iLogger.Log(KLogWarning, "[%s] connection %d not found on user command.", iName.c_str(), connectionId);
	else
		i->second->ProcessCommand(message);
}

void Zone::ProcessUserChannelChat(SSConnection & connection, const bstring & packet)
{
	reader_t reader(packet, 1);

	uint32 connectionId = reader.ReadUint32();
	std::string channel = reader.ReadString(32);
	std::string message = reader.ReadString();

	PlayerMap::iterator i = iPlayers.find(connectionId);
	if(i == iPlayers.end())
		iLogger.Log(KLogWarning, "[%s] connection %d not found on chat message.", iName.c_str(), connectionId);
	else
	{
		uint32 chatOrdinal = 1;
		if(!channel.empty() && sscanf(channel.c_str(), "%d", &chatOrdinal) != 1)
			iLogger.Log(KLogWarning, "[%s]{%s} invalid chat channel number.", iName.c_str(), i->second->GetName().c_str());
		else
			i->second->ProcessChatMessage(chatOrdinal, message);
	}
}

void Zone::ProcessServerCapabilities(SSConnection & connection, const bstring & packet)
{
	reader_t reader(packet, 1);

	uint32 capabilities = reader.ReadUint32();

	if(capabilities & KMulticastChat)
		iCanMulticastChat = true;
	if(capabilities & KSupportsDemographics)
		iSupportsDemographics = true;
}

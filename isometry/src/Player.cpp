#include "base.h"
#include "Player.h"
#include "Database.h"
#include "LogLib.h"
#include "SSNetLib.h"
#include "Zone.h"
#include "Chat.h"
#include "Squad.h"
#include "ZoneManager.h"
#include "ChatManager.h"
#include "SquadManager.h"
#include "PlayerManager.h"
#include "Ban.h"
#include "Message.h"
#include "BanFree.h"
#include "version.h"

Player::Player(Zone & zone, const std::string & name, uint32 connectionId, uint32 scoreId, const InetAddress & ipAddress, uint32 machineId, const BanData & banData)
	: iZone(zone),
	  iConnectionId(connectionId),
	  iScoreId(scoreId),
	  iName(name),
	  iIpAddress(ipAddress),
	  iMachineId(machineId),
	  iBanData(banData),
	  iSquad(0),
	  iReadMessages(false)
{
	Database::PlayerGetInfo info(name, scoreId);
	info.Execute();

	iId = info.GetId();
	iSecondsPlayed = info.GetSecondsPlayed();
	iCreationDate = info.GetCreationDate();
	iBanner = info.GetBanner();
	iScore.iWins = info.GetWins();
	iScore.iLosses = info.GetLosses();
	iScore.iGoals = info.GetGoals();
	iScore.iPoints = info.GetPoints();
	iScore.iFlagPoints = info.GetFlagPoints();

	PlayerManager::Instance().Register(*this);

	if(info.HasSquad())
		try
		{
			iSquad = SquadManager::Instance().GetById(info.GetSquadId());
			iSquad->Login(*this);
		}
		catch(const not_found_error &)
		{ }

	Database::PlayerGetChatSubscriptions subscriptions(GetId());
	subscriptions.Execute();

	iChannels.resize(10);
	for(uint32 i = 0; i < 10; ++i)
		if(subscriptions.HasSubscription(i))
		{
			try
			{
				iChannels[i] = ChatManager::Instance().GetById(subscriptions.GetSubscription(i));
				iChannels[i]->Login(*this);
			}
			catch(const not_found_error &)
			{
				Logger::Instance().Log(KLogError, "[%s]{%s} database inconsistency: chat channel %d.", iZone.GetName().c_str(), GetName().c_str(), i);
			}
		}
}

Player::~Player()
{
	Time now;
	uint32 seconds = static_cast <uint32> (now.SecondsSinceUtcEpoch() - iSessionStartTime.SecondsSinceUtcEpoch());

	if(iSquad != 0)
		iSquad->Logout(*this);

	for(Channels::iterator i = iChannels.begin(); i != iChannels.end(); ++i)
		if(*i != 0)
			(*i)->Logout(*this);

	//
	// Clear out all of the messages if they've been read in this session.
	//
	if(iReadMessages)
		Database::MessageDeleteAll(GetId()).Execute();

	Database::PlayerLogout(iId, seconds).Execute();
	PlayerManager::Instance().Unregister(*this);
}

bool Player::IsValidName(const std::string & name)
{
	return (!name.empty() && name.length() <= 24 && name.find(':') == std::string::npos && name[0] != '^' && name.find(")>") == std::string::npos);
}

void Player::Poll(SSConnection & connection)
{
}

void Player::SendMessage(const std::string & message)
{
	DataWriter <LittleEndian, AsciiZ> writer;

	writer.AppendUint8(0x09);
	writer.AppendUint32(iConnectionId);
	writer.AppendString(message);

	iZone.Send(writer.Asbstring());
}

void Player::SendMessage(const std::string & prefix, const StringSequence & strings, std::string::value_type join, std::string::size_type maxLength)
{
	std::string::size_type length = prefix.length();

	StringSequence accumulator;
	for(StringSequence::size_type i = 0; i < strings.size(); ++i)
	{
		if(length + strings[i].length() > maxLength)
		{
			SendMessage(prefix + String::Join(accumulator, join));
			accumulator.clear();
		}
		else
			accumulator.push_back(strings[i]);
	}

	if(!accumulator.empty())
		SendMessage(prefix + String::Join(accumulator, join));
}

void Player::SendChatMessage(const Chat & chat, const std::string & message)
{
	for(uint32 i = 0; i < iChannels.size(); ++i)
		if(iChannels[i] == &chat)
		{
			DataWriter <LittleEndian, AsciiZ> writer;

			writer.AppendUint8(0x0A);
			writer.AppendUint32(iConnectionId);
			writer.AppendUint8(i);
			writer.AppendString(message);

			iZone.Send(writer.Asbstring());
			break;
		}
}

void Player::SendRemotePlayerMessage(const std::string & message)
{
	DataWriter <LittleEndian, AsciiZ> writer;

	writer.AppendUint8(0x03);
	writer.AppendUint32(0x00); // This is supposed to be the server id from which the message was sent
	writer.AppendUint8(0x02);
	writer.AppendUint8(0x00);
	writer.AppendString(message);

	iZone.Send(writer.Asbstring());
}

void Player::SendRawPacket(const bstring & packet)
{
	DataWriter <LittleEndian> writer;
	writer.AppendUint8(0x32);
	writer.AppendUint32(GetConnectionId());
	writer.AppendString(packet);

	iZone.Send(writer.Asbstring());
}

void Player::Kickoff(const EKickoffReason & reason)
{
	DataWriter <LittleEndian> writer;

	writer.AppendUint8(0x08);
	writer.AppendUint32(GetConnectionId());
	writer.AppendUint16(reason);

	iZone.Send(writer.Asbstring());
}

const Zone & Player::GetZone() const
{
	return iZone;
}

uint32 Player::GetId() const
{
	return iId;
}

uint32 Player::GetConnectionId() const
{
	return iConnectionId;
}

uint32 Player::GetScoreId() const
{
	return iScoreId;
}

const std::string & Player::GetName() const
{
	return iName;
}

const std::string & Player::GetSquadName() const
{
	static std::string nullstr;
	return (iSquad ? iSquad->GetName() : nullstr);
}

const bstring & Player::GetBanner() const
{
	return iBanner;
}

uint32 Player::GetSecondsPlayed() const
{
	return iSecondsPlayed;
}

const std::string & Player::GetCreationDate() const
{
	return iCreationDate;
}

const Player::Score & Player::GetScore() const
{
	return iScore;
}

InetAddress Player::GetIpAddress() const
{
	return iIpAddress;
}

uint32 Player::GetMachineId() const
{
	return iMachineId;
}

const BanData & Player::GetBanData() const
{
	return iBanData;
}

void Player::UpdateBanner(const bstring & banner)
{
	iBanner = banner;
	Database::PlayerUpdateBanner(iId, banner).Execute();
}

void Player::UpdateScore(const Score & score)
{
	iScore = score;
	Database::PlayerUpdateScore(iScoreId, iId, iScore.iWins, iScore.iLosses, iScore.iGoals, iScore.iPoints, iScore.iFlagPoints).Execute();
}

void Player::ChatKicked(const Chat & chat)
{
	for(Channels::iterator i = iChannels.begin(); i != iChannels.end(); ++i)
		if(*i == &chat)
		{
			SendMessage(Format("You have been kicked from the chat %s", chat.GetName().c_str()));
			*i = 0;
			break;
		}
}

void Player::ChatDeleted(const Chat & chat)
{
	for(Channels::iterator i = iChannels.begin(); i != iChannels.end(); ++i)
		if(*i == &chat)
		{
			SendMessage(Format("The chat %s has been deleted by the owner.", chat.GetName().c_str()));
			*i = 0;
			break;
		}
}

void Player::SquadKicked()
{
	SendMessage("You have been kicked from your squad.");
	iSquad = 0;
}

void Player::SquadDeleted()
{
	SendMessage("Your squad has been deleted by the owner.");
	iSquad = 0;
}

void Player::BanKicked(bool kickoff)
{
	SendMessage(GetZone().GetBanText());

	if(kickoff)
		Kickoff(KKickBanned);
}

void Player::ProcessChatMessage(uint32 ordinal, const std::string & message)
{
	if(ordinal < 0 || ordinal >= iChannels.size())
	{
		Logger::Instance().Log(KLogWarning, "[%s]{%s} invalid channel ordinal (%d).", iZone.GetName().c_str(), GetName().c_str(), ordinal);
		return;
	}

	Chat * chat = iChannels[ordinal];
	if(chat == 0)
	{
		Logger::Instance().Log(KLogWarning, "[%s]{%s} not subscribed to channel %d.", iZone.GetName().c_str(), GetName().c_str(), ordinal);
		return;
	}

	chat->SendMessage(*this, message);
}

void Player::ProcessCommand(const std::string & message)
{
	if(message.size() > 2 && message[1] == '*')
	{
		ProcessBillerMessage(message);
		return;
	}

	int8 buffer[256] = { 0 }, buffer2[256] = { 0 }, buffer3[256] = { 0 };
	sscanf(message.c_str(), "%s %s %[^\n]", buffer, buffer2, buffer3);

	std::string command = String::TrimCopy(buffer);
	std::string subcommand = String::TrimCopy(buffer2);
	std::string arguments = String::TrimCopy(buffer3);

	//
	// Ignore commands of the form ?blah=. This is primarily to ignore the
	// ?chat= command that Continuum sends on login.
	//
	if(command.substr(0, 6) == "?chat=")
		return;

	if(command == "?chat")
	{
		std::string chat, player;
		if(sscanf(arguments.c_str(), " %[^:] : %[^\n]", buffer2, buffer3) == 2)
		{
			chat = String::TrimCopy(buffer2);
			player = String::TrimCopy(buffer3);
		}

		if(subcommand.empty())
			ProcessChat();
		else if(subcommand == "create")
			ProcessChatCreate(arguments);
		else if(subcommand == "delete")
			ProcessChatDelete(arguments);
		else if(subcommand == "grant")
			ProcessChatGrant(chat, player);
		else if(subcommand == "allow")
			ProcessChatAllow(chat, player);
		else if(subcommand == "deny")
			ProcessChatDeny(chat, player);
		else if(subcommand == "mode")
			ProcessChatMode(chat, player);
		else if(subcommand == "join")
		{
			uint32 ordinal;
			if(sscanf(arguments.c_str(), "%d %[^\n]", &ordinal, buffer) != 2)
				SendMessage("Error: invalid chat join syntax.");
			else
				ProcessChatJoin(ordinal, String::TrimCopy(buffer));
		}
		else if(subcommand == "leave")
			ProcessChatLeave(arguments);
		else if(subcommand == "list")
			ProcessChatList();
		else if(subcommand == "info")
			ProcessChatInfo(arguments);
		else if(subcommand == "find")
			ProcessChatFind(arguments);
		else if(!subcommand.empty())
		{
			sscanf(message.c_str(), "%*s %[^\n]", buffer);
			ProcessChatJoinList(String::TrimCopy(buffer));
		}
	}
	else if(command == "?squad")
	{
		if(subcommand.empty())
			ProcessSquad();
		else if(subcommand == "create")
			ProcessSquadCreate(arguments);
		else if(subcommand == "delete")
			ProcessSquadDelete(arguments);
		else if(subcommand == "join")
			ProcessSquadJoin(arguments);
		else if(subcommand == "leave")
			ProcessSquadLeave();
		else if(subcommand == "grant")
			ProcessSquadGrant(arguments);
		else if(subcommand == "allow")
			ProcessSquadAllow(arguments);
		else if(subcommand == "deny")
			ProcessSquadDeny(arguments);
		else if(subcommand == "mode")
			ProcessSquadMode(arguments);
		else if(subcommand == "list")
			ProcessSquadList();
		else if(subcommand == "listmembers")
			ProcessSquadListMembers(arguments);
	}
	else if(command == "?addop")
	{
		uint32 access, offset;
		if(sscanf(message.c_str(), "%*s -* %d %n", &access, &offset) == 1)
			ProcessAddOp(access, true, String::TrimCopy(message.c_str() + offset));
		else if(sscanf(message.c_str(), "%*s %d %n", &access, &offset) == 1)
			ProcessAddOp(access, false, String::TrimCopy(message.c_str() + offset));
		else
			SendMessage("Usage: ?addop [-*] <level#> <playerName>");
	}
	else if(command == "?removeop")
	{
		if(sscanf(message.c_str(), "%*s -* %[^\n]", buffer) == 1)
			ProcessRemoveOp(String::TrimCopy(buffer), true);
		else
		{
			sscanf(message.c_str(), "%*s %[^\n]", buffer);
			ProcessRemoveOp(String::TrimCopy(buffer), false);
		}
	}
	else if(command == "?listop")
		ProcessListOp(message.find("-*") != std::string::npos);
	else if(command == "?ban" || command == "?changeban")
	{
		bool changeBan = command == "?changeban";

		bool netBan = false;
		bool banAllInRange = false;
		bool removeIdNumber = false;
		bool dontKick = false;
		bool banInPlayerZone = false;
		uint32 banDayCount = 7;
		uint32 access = 4;
		InetAddress ipAddress;
		uint32 ipMask = 24;
		uint32 machineId = 0;

		bool * pbanAllInRange = 0;
		bool * premoveIdNumber = 0;
		uint32 * pbanDayCount = 0;
		uint32 * paccess = 0;
		InetAddress * pipAddress = 0;
		uint32 * pipMask = 0;
		uint32 * pmachineId = 0;

		try
		{
			uint32 here, hereAccum;
			sscanf(message.c_str(), "%*s %n", &here);
			while(sscanf(message.c_str() + here, " %s %n", buffer, &hereAccum) >= 1 && buffer[0] == '-')
			{
				here += hereAccum;
				command = buffer;

				if(command == "-*")
					netBan = true;
				else if(command == "-!")
					banAllInRange = true, pbanAllInRange = &banAllInRange;
				else if(command == "-c")
					removeIdNumber = true, premoveIdNumber = &removeIdNumber;
				else if(command == "-k")
					dontKick = true;
				else if(command == "-z")
					banInPlayerZone = true;
				else if(command.length() < 3)
					throw malformed_error("switch", command);
				else if(command[1] == 'e')
				{
					if(sscanf(buffer + 2, "%u", &banDayCount) != 1)
						throw malformed_error("switch", command);
					pbanDayCount = &banDayCount;
				}
				else if(command[1] == 'a')
				{
					if(sscanf(buffer + 2, "%u", &access) != 1)
						throw malformed_error("switch", command);
					paccess = &access;
				}
				else if(command[1] == 'i')
				{
					try
					{
						ipAddress = InetAddress(buffer + 2);
						pipAddress = &ipAddress;
					}
					catch(const not_found_error &)
					{
						throw malformed_error("switch", buffer);
					}
				}
				else if(command[1] == 'm')
				{
					if(sscanf(buffer + 2, "%u", &ipMask) != 1)
						throw malformed_error("switch", command);
					pipMask = &ipMask;
				}
				else if(command[1] == 'n')
				{
					if(sscanf(buffer + 2, "%u", &machineId) != 1)
						throw malformed_error("switch", command);
					pmachineId = &machineId;
				}
				else
					throw malformed_error("switch", command);
			}

			if(!changeBan)
			{
				if(sscanf(message.c_str() + here, "%[^:]:%[^\n]", buffer, buffer2) == 2)
					ProcessBan(String::TrimCopy(buffer), String::TrimCopy(buffer2), netBan, banAllInRange, removeIdNumber, dontKick, banInPlayerZone, banDayCount, access, ipAddress, ipMask, machineId);
				else
				{
					sscanf(message.c_str() + here, "%[^\n]", buffer);
					ProcessBan(String::TrimCopy(buffer), std::string(), netBan, banAllInRange, removeIdNumber, dontKick, banInPlayerZone, banDayCount, access, ipAddress, ipMask, machineId);
				}
			}
			else
			{
				uint32 banId;
				if(sscanf(message.c_str() + here, "%d", &banId) != 1)
					throw malformed_error("ban id", message.c_str() + here);
				ProcessChangeBan(banId, pbanAllInRange, premoveIdNumber, pbanDayCount, paccess, pipAddress, pipMask, pmachineId);
			}
		}
		catch(const malformed_error & e)
		{
			SendMessage(Format("Error: %s.", e.what()));
		}
	}
	else if(command == "?liftban")
	{
		uint32 banId;
		if(sscanf(message.c_str(), "%*s %d", &banId) == 1)
			ProcessLiftBan(banId);
		else
			SendMessage("Usage: ?liftban <ban ID>");
	}
	else if(command == "?listban")
	{
		uint32 limit = 10;
		sscanf(message.c_str(), "%*s %*[^0-9]%d", &limit);
		ProcessListBan(message.find("-*") != std::string::npos, message.find("-d") != std::string::npos, limit);
	}
	else if(command == "?bancomment")
	{
		uint32 banId;
		if(sscanf(message.c_str(), "%*s %d %[^\n]", &banId, buffer) == 2)
			ProcessBanComment(banId, String::TrimCopy(buffer));
		else
			SendMessage("Usage: ?bancomment <ban ID> <comment>");
	}
	else if(command == "?banfree")
	{
		uint32 access = 4;
		bool netwide = false;

		try
		{
			uint32 here, hereAccum;
			sscanf(message.c_str(), "%*s %n", &here);
			while(sscanf(message.c_str() + here, "%s %n", buffer, &hereAccum) >= 1 && buffer[0] == '-')
			{
				here += hereAccum;
				command = buffer;

				if(command == "-*")
					netwide = true;
				else if(command[1] == 'a')
				{
					if(sscanf(buffer + 2, "%d", &access) != 1)
						throw malformed_error("switch", command);
				}
				else
					throw malformed_error("switch", command);
			}
			ProcessBanFree(String::TrimCopy(message.c_str() + here), access, netwide);
		}
		catch(const malformed_error &)
		{
			SendMessage("Usage: ?banfree [-*] [-a#] <name>");
		}
	}
	else if(command == "?removebanfree")
	{
		if(sscanf(message.c_str(), "%*s -* %[^\n]", buffer) == 1)
			ProcessRemoveBanFree(String::TrimCopy(buffer), true);
		else
		{
			sscanf(message.c_str(), "%*s %[^\n]", buffer);
			ProcessRemoveBanFree(String::TrimCopy(buffer), false);
		}
	}
	else if(command == "?listbanfree")
	{
		uint32 limit = 10;
		sscanf(message.c_str(), "%*s %*[^0-9]%d", &limit);
		ProcessListBanFree(message.find("-*") != std::string::npos, limit);
	}
	else if(command == "?bantext")
	{
		if(sscanf(message.c_str(), "%*s %[^\n]", buffer) != 1)
			SendMessage(GetZone().GetBanText());
		else
		{
			try
			{
				std::string text = String::TrimCopy(buffer);
				Database::ZoneUpdateBanText(GetId(), GetZone().GetId(), text).Execute();
				iZone.UpdateBanText(text);
				SendMessage("Ban text updated.");
			}
			catch(const permission_error &)
			{
				SendMessage("Error: you do not have permission to update the zone's ban text.");
			}
		}
	}
	else if(command == "?message")
	{
		if(sscanf(message.c_str(), "%*s %[^:]:%[^\n]", buffer, buffer2) != 2)
			SendMessage("Usage: ?message username : message");
		else
			ProcessMessage(String::TrimCopy(buffer), String::TrimCopy(buffer2));
	}
	else if(command == "?messages")
		ProcessMessages();
	else if(command == "?password")
	{
		sscanf(message.c_str(), "%*s %[^\n]", buffer);
		ProcessSetPassword(String::TrimCopy(buffer));
	}
	else if(command == "?alias")
	{
		sscanf(message.c_str(), "%*s %[^\n]", buffer);
		ProcessAlias(String::TrimCopy(buffer));
	}
	else if(command == "?find")
	{
		sscanf(message.c_str(), "%*s %[^\n]", buffer);
		ProcessFind(String::TrimCopy(buffer));
	}
	else if(command == "?bzone")
		ProcessZone();
	else if(command == "?zones")
		ProcessZones();
	else if(command == "?bversion")
		ProcessVersion();
	else if(command == "?top10")
	{
		if(subcommand == "points")
			ProcessTop10Points();
		else if(subcommand == "kills")
			ProcessTop10Kills();
		else if(subcommand == "deaths")
			ProcessTop10Deaths();
		else if(subcommand == "squad")
		{
			if(arguments == "points")
				ProcessTop10SquadPoints();
			else if(arguments == "kills")
				ProcessTop10SquadKills();
			else if(arguments == "deaths")
				ProcessTop10SquadDeaths();
		}
	}
	else if(command == "?help")
		SendMessage("See the command reference at: http://docs.google.com/Doc?id=d67vd23_3775b98kcr");
	else
		SendMessage("Unknown biller command " + command);
}

void Player::ProcessBillerMessage(const std::string & message)
{
	bool canSend = false;
	Database::PlayerGetPermission(GetId(), GetZone().GetId(), canSend).Execute();

	if(!canSend)
	{
		SendMessage("You do not have permission to send biller-wide messages.");
		return;
	}

	ZoneManager & zones = ZoneManager::Instance();

	for(ZoneManager::iterator i = zones.begin(); i != zones.end(); ++i)
		i->second->SendMessage(message.substr(2));
}

void Player::ProcessChat()
{
	for(Channels::iterator i = iChannels.begin(); i != iChannels.end(); ++i)
	{
		if(*i == 0)
			continue;

		const Chat::ActiveMemberList & members = (*i)->GetActiveMembers();

		StringSequence memberNames;
		for(Chat::ActiveMemberList::iterator j = members.begin(); j != members.end(); ++j)
			memberNames.push_back((*j)->GetName());

		SendMessage(Format("%s: ", (*i)->GetName().c_str()), memberNames, ',', 80);
	}
}

void Player::ProcessChatCreate(const std::string & chat)
{
	if(!Chat::IsValidName(chat))
	{
		SendMessage(Format("Error: invalid chat name %s.", chat.c_str()));
		return;
	}

	try
	{
		uint32 chatId;
		Database::ChatCreate(chat, GetId(), chatId).Execute();
		SendMessage(Format("Chat %s successfully created.", chat.c_str()));
	}
	catch(const constraint_error &)
	{
		SendMessage(Format("Error: chat %s already exists.", chat.c_str()));
	}
}

void Player::ProcessChatDelete(const std::string & chat)
{
	if(!Chat::IsValidName(chat))
	{
		SendMessage(Format("Error: invalid chat name %s.", chat.c_str()));
		return;
	}

	try
	{
		Chat * chatObj = ChatManager::Instance().GetByName(chat);
		chatObj->Delete(*this);

		for(Channels::iterator i = iChannels.begin(); i != iChannels.end(); ++i)
			if(*i == chatObj)
			{
				*i = 0;
				break;
			}

		SendMessage(Format("Chat %s successfully deleted.", chat.c_str()));
	}
	catch(const not_found_error &)
	{
		SendMessage(Format("Error: chat %s does not exist.", chat.c_str()));
	}
	catch(const permission_error &)
	{
		SendMessage(Format("Error: you do not have permission to delete the chat %s.", chat.c_str()));
	}
}

void Player::ProcessChatGrant(const std::string & chat, const std::string & player)
{
	if(!Player::IsValidName(player))
	{
		SendMessage(Format("Error: invalid player name %s.", player.c_str()));
		return;
	}

	try
	{
		Chat * chatObj = ChatManager::Instance().GetByName(chat);
		chatObj->Grant(*this, player);
		SendMessage(Format("Chat %s successfully granted to %s.", chat.c_str(), player.c_str()));
	}
	catch(const not_found_error &)
	{
		SendMessage(Format("Error: could not find player %s.", player.c_str()));
	}
	catch(const permission_error &)
	{
		SendMessage(Format("Error: you do not have permission to grant the chat %s.", chat.c_str()));
	}
}

void Player::ProcessChatAllow(const std::string & chat, const std::string & player)
{
	if(!Chat::IsValidName(chat))
	{
		SendMessage(Format("Error: invalid chat name %s.", chat.c_str()));
		return;
	}

	if(!Player::IsValidName(player))
	{
		SendMessage(Format("Error: invalid player name %s.", player.c_str()));
		return;
	}

	Chat * chatObj = 0;
	try
	{
		chatObj = ChatManager::Instance().GetByName(chat);
	}
	catch(const not_found_error &)
	{
		SendMessage(Format("Error: chat %s does not exist.", chat.c_str()));
		return;
	}

	try
	{
		chatObj->Allow(*this, player);
		SendMessage(Format("Player %s successfully granted permission to join the chat %s.", player.c_str(), chat.c_str()));
	}
	catch(const not_found_error &)
	{
		SendMessage(Format("Error: could not find player %s.", player.c_str()));
	}
	catch(const permission_error &)
	{
		SendMessage("Error: you do not have permission to allow players into this chat.");
	}
	catch(const constraint_error &)
	{
		SendMessage(Format("Error: player %s is already allowed to join the chat %s.", player.c_str(), chat.c_str()));
	}
}

void Player::ProcessChatDeny(const std::string & chat, const std::string & player)
{
	if(!Chat::IsValidName(chat))
	{
		SendMessage(Format("Error: invalid chat name %s.", chat.c_str()));
		return;
	}

	if(!Player::IsValidName(player))
	{
		SendMessage(Format("Error: invalid player name %s.", player.c_str()));
		return;
	}

	Chat * chatObj = 0;
	try
	{
		chatObj = ChatManager::Instance().GetByName(chat);
	}
	catch(const not_found_error &)
	{
		SendMessage(Format("Error: chat %s does not exist.", chat.c_str()));
		return;
	}

	try
	{
		chatObj->Deny(*this, player);
		SendMessage(Format("%s successfully denied permission to join the chat.", player.c_str()));
	}
	catch(const not_found_error &)
	{
		SendMessage(Format("Error: could not find player %s.", player.c_str()));
	}
	catch(const permission_error &)
	{
		SendMessage("Error: you do not have permission to deny players from this chat.");
	}
}

void Player::ProcessChatMode(const std::string & chat, const std::string & mode)
{
	if(!Chat::IsValidName(chat))
	{
		SendMessage(Format("Error: invalid chat name %s.", chat.c_str()));
		return;
	}

	try
	{
		Chat * chatObj = ChatManager::Instance().GetByName(chat);

		if(mode.empty())
			SendMessage(Format("Chat is currently '%s'", chatObj->IsPublic() ? "public" : "private"));
		else if(String::Compare(mode, "public", String::KCaseInsensitive) == 0)
		{
			chatObj->SetPublic(*this, true);
			SendMessage("Chat is now open to all players.");
		}
		else if(String::Compare(mode, "private", String::KCaseInsensitive) == 0)
		{
			chatObj->SetPublic(*this, false);
			SendMessage("Chat is now restricted to selected members.");
		}
		else
			SendMessage("Error: invalid chat mode.");
	}
	catch(const not_found_error &)
	{
		SendMessage(Format("Error: chat %s does not exist.", chat.c_str()));
	}
	catch(const permission_error &)
	{
		SendMessage("Error: you do not have permission to change the chat mode.");
	}
}

void Player::ProcessChatJoin(uint32 ordinal, const std::string & chat)
{
	if(!Chat::IsValidName(chat))
	{
		SendMessage(Format("Error: invalid chat name %s.", chat.c_str()));
		return;
	}

	if(ordinal < 0 || ordinal >= iChannels.size())
	{
		SendMessage(Format("Error: invalid channel number %d.", ordinal));
		return;
	}

	try
	{
		Chat * leaveChat = iChannels[ordinal];
		Chat * chatObj = ChatManager::Instance().GetByName(chat);

		//
		// If setting the same chat on the same channel, no error.
		//
		if(chatObj == leaveChat)
			return;

		chatObj->Join(*this, ordinal);

		iChannels[ordinal] = chatObj;

		if(leaveChat != 0)
			leaveChat->Leave(*this);

		SendMessage(Format("You have successfully joined the chat %s on channel %d.", chat.c_str(), ordinal));
	}
	catch(const permission_error &)
	{
		SendMessage(Format("Error: you do not have permission to join the chat %s.", chat.c_str()));
	}
	catch(const not_found_error &)
	{
		SendMessage(Format("Error: chat %s does not exist.", chat.c_str()));
	}
	catch(const constraint_error &)
	{
		SendMessage(Format("Error: you are already on chat %s.", chat.c_str()));
	}
}

void Player::ProcessChatLeave(const std::string & chat)
{
	if(!Chat::IsValidName(chat))
	{
		SendMessage(Format("Error: invalid chat name %s.", chat.c_str()));
		return;
	}

	for(uint32 i = 0; i < iChannels.size(); ++i)
		if(iChannels[i] != 0 && chat == *iChannels[i])
		{
			iChannels[i]->Leave(*this);
			iChannels[i] = 0;

			SendMessage(Format("You have successfully left the chat %s.", chat.c_str()));
			return;
		}

	SendMessage(Format("Error: you are not on chat %s.", chat.c_str()));
}

void Player::ProcessChatInfo(const std::string & chat)
{
	bool foundChat = false;
	for(uint32 i = 0; i < iChannels.size(); ++i)
		if(iChannels[i] != 0 && (chat.empty() || chat == *iChannels[i]))
		{
			SendMessage(Format("Channel %d: %s", i, iChannels[i]->GetName().c_str()));
			SendMessage(Format("Owner: %s", iChannels[i]->GetOwnerName().c_str()));
			SendMessage(Format("Mode: %s", (iChannels[i]->IsPublic() ? "public" : "private")));
			foundChat = true;
		}

	if(!foundChat)
	{
		if(chat.empty())
			SendMessage("You are not on any chats.");
		else
			SendMessage(Format("You are not on chat %s.", chat.c_str()));
	}
}

void Player::ProcessChatFind(const std::string & player)
{
	if(!Player::IsValidName(player))
	{
		SendMessage(Format("Error: invalid player name %s.", player.c_str()));
		return;
	}

	StringSequence chats;
	for(Channels::iterator i = iChannels.begin(); i != iChannels.end(); ++i)
	{
		if(*i == 0)
			continue;

		const Chat::ActiveMemberList & members = (*i)->GetActiveMembers();
		for(Chat::ActiveMemberList::const_iterator j = members.begin(); j != members.end(); ++j)
			if(player == **j)
			{
				chats.push_back((*i)->GetName());
				break;
			}
	}
	if(chats.empty())
		SendMessage(Format("Player %s not found in any subscribed chats.", player.c_str()));
	else
		SendMessage(Format("%s: ", player.c_str()), chats, ',', 80);
}

void Player::ProcessChatJoinList(const std::string & chatlist)
{
	StringSequence strs = String::Split(chatlist, ',');

	//
	// Always stay on the same channel 0 through this command.
	//
	if(iChannels[0] != 0)
		strs.insert(strs.begin(), iChannels[0]->GetName());
	else
		strs.insert(strs.begin(), "");

	if(strs.size() > iChannels.size())
	{
		SendMessage("Error: too many chats specified.");
		return;
	}

	for(uint32 i = 0; i < strs.size(); ++i)
		String::Trim(strs[i]);

	//
	// Our beautiful n^2 algorithm to check for duplicates. This isn't so
	// bad since n is approx 10.
	//
	for(uint32 i = 0; i < strs.size() - 1; ++i)
		if(strs[i].empty())
			continue;
		else
			for(uint32 j = i + 1; j < strs.size(); ++j)
				if(String::Compare(strs[i], strs[j], String::KCaseInsensitive) == 0)
				{
					SendMessage("Error: duplicate chats specified.");
					return;
				}

	for(uint32 i = 0; i < iChannels.size(); ++i)
		if(iChannels[i] != 0)
		{
			iChannels[i]->Leave(*this);
			iChannels[i] = 0;
		}

	for(uint32 i = 0; i < strs.size(); ++i)
		if(!strs[i].empty())
			ProcessChatJoin(i, strs[i]);
}

void Player::ProcessChatList()
{
	Database::PlayerListChats list(iId);
	list.Execute();

	if(list.empty())
		SendMessage("You do not own any chats.");
	else
	{
		SendMessage("Chats Owned:");
		for(Database::PlayerListChats::iterator i = list.begin(); i != list.end(); ++i)
			SendMessage(Format("  %s", i->c_str()));
	}
}

void Player::ProcessSquad()
{
	if(iSquad == 0)
		SendMessage("You are not on any squad.");
	else
	{
		SendMessage("Squad: " + iSquad->GetName());
		SendMessage("Owner: " + iSquad->GetOwnerName());
		SendMessage(Format("Mode: %s", (iSquad->IsPublic() ? "public" : "private")));
	}
}

void Player::ProcessSquadCreate(const std::string & name)
{
	if(!Squad::IsValidName(name))
	{
		SendMessage("Error: invalid squad name.");
		return;
	}

	try
	{
		uint32 squadId;
		Database::SquadCreate(name, iId, squadId).Execute();
		SendMessage("Squad successfully created.");
	}
	catch(const constraint_error &)
	{
		SendMessage("Error: squad already exists.");
	}
}

void Player::ProcessSquadDelete(const std::string & name)
{
	if(!Squad::IsValidName(name))
	{
		SendMessage("Error: invalid squad name.");
		return;
	}

	try
	{
		Squad * squad = SquadManager::Instance().GetByName(name);
		squad->Delete(*this);

		if(squad == iSquad)
			iSquad = 0;

		SendMessage("Squad successfully deleted.");
	}
	catch(const not_found_error &)
	{
		SendMessage("Error: squad does not exist.");
	}
	catch(const permission_error &)
	{
		SendMessage("Error: only the squad owner can delete the squad.");
	}
}

void Player::ProcessSquadGrant(const std::string & player)
{
	if(iSquad == 0)
	{
		SendMessage("Error: you are not currently on a squad.");
		return;
	}

	if(!Player::IsValidName(player))
	{
		SendMessage("Error: invalid player name.");
		return;
	}

	try
	{
		iSquad->Grant(*this, player);
		SendMessage("Squad successfully granted.");
	}
	catch(const not_found_error &)
	{
		SendMessage("Error: could not find player.");
	}
	catch(const permission_error &)
	{
		SendMessage("Error: you do not have permission to grant this squad.");
	}
}

void Player::ProcessSquadJoin(const std::string & squad)
{
	if(!Squad::IsValidName(squad))
	{
		SendMessage("Error: invalid squad name.");
		return;
	}

	try
	{
		Squad * target = SquadManager::Instance().GetByName(squad);
		target->Join(*this);

		if(iSquad != 0)
			iSquad->Leave(*this);

		iSquad = target;
		SendMessage("You have successfully joined the squad.");
	}
	catch(const not_found_error &)
	{
		SendMessage("Error: squad does not exist.");
	}
	catch(const permission_error &)
	{
		SendMessage("Error: you are not allowed to join this squad.");
	}
}

void Player::ProcessSquadLeave()
{
	if(iSquad == 0)
		SendMessage("You are not on any squad.");
	else
	{
		iSquad->Leave(*this);
		iSquad = 0;
		SendMessage("You have successfully left the squad.");
	}
}

void Player::ProcessSquadAllow(const std::string & player)
{
	if(iSquad == 0)
	{
		SendMessage("Error: you are not in any squad.");
		return;
	}

	if(!Player::IsValidName(player))
	{
		SendMessage("Error: invalid player name.");
		return;
	}

	try
	{
		iSquad->Allow(*this, player);
		SendMessage("Player successfully granted permission to join squad.");
	}
	catch(const not_found_error &)
	{
		SendMessage("Error: could not find player.");
	}
	catch(const permission_error &)
	{
		SendMessage("Error: you do not have permission to add users to this squad.");
	}
	catch(const constraint_error &)
	{
		SendMessage("Error: player is already allowed to join squad.");
	}
}

void Player::ProcessSquadDeny(const std::string & player)
{
	if(iSquad == 0)
	{
		SendMessage("Error: you are not in any squad.");
		return;
	}

	if(!Player::IsValidName(player))
	{
		SendMessage("Error: invalid player name.");
		return;
	}

	try
	{
		iSquad->Deny(*this, player);
		SendMessage("Player successfully denied permission from joining squad.");
	}
	catch(const not_found_error &)
	{
		SendMessage("Error: could not find player.");
	}
	catch(const permission_error &)
	{
		SendMessage("Error: you do not have permission to deny users from this squad.");
	}
}

void Player::ProcessSquadMode(const std::string & mode)
{
	if(iSquad == 0)
	{
		SendMessage("Error: you are not in any squad.");
		return;
	}

	try
	{
		if(mode.empty())
			SendMessage(Format("Squad is currently '%s'", (iSquad->IsPublic() ? "public" : "private")));
		else if(String::Compare(mode, "public", String::KCaseInsensitive) == 0)
		{
			iSquad->SetPublic(*this, true);
			SendMessage("Squad is now open to all players.");
		}
		else if(String::Compare(mode, "private", String::KCaseInsensitive) == 0)
		{
			iSquad->SetPublic(*this, false);
			SendMessage("Squad is now restricted to selected members.");
		}
		else
			SendMessage("Error: invalid squad mode.");
	}
	catch(const permission_error &)
	{
		SendMessage("Error: you do not have permission to change the squad mode.");
	}
}

void Player::ProcessSquadOwner(const std::string & squad)
{
	if(squad.empty())
		if(iSquad != 0)
			SendMessage(iSquad->GetOwnerName());
		else
			SendMessage("Error: no squad specified.");
	else
		try
		{
			Squad * s = SquadManager::Instance().GetByName(squad);
			SendMessage(s->GetOwnerName());
		}
		catch(const not_found_error &)
		{
			SendMessage("Error: squad not found.");
		}
}

void Player::ProcessSquadList()
{
	Database::PlayerListSquads list(iId);
	list.Execute();

	if(list.empty())
		SendMessage("You do not own any squads.");
	else
	{
		SendMessage("Squads Owned:");
		for(Database::PlayerListSquads::iterator i = list.begin(); i != list.end(); ++i)
			SendMessage(Format("  %s", i->c_str()));
	}
}

void Player::ProcessSquadListMembers(const std::string & squad)
{
	if(!squad.empty() && !Squad::IsValidName(squad))
	{
		SendMessage("Error: invalid squad name.");
		return;
	}

	if(squad.empty() && iSquad == 0)
	{
		SendMessage("Error: you are not on any squad.");
		return;
	}

	try
	{
		Squad * s = iSquad;
		if(!squad.empty())
			s = SquadManager::Instance().GetByName(squad);
		
		const Squad::MemberList & members = s->GetMembers();
		if(members.empty())
			SendMessage("There are no members in that squad.");
		else
		{
			SendMessage("Squad Members:");
			for(Squad::MemberList::iterator i = members.begin(); i != members.end(); ++i)
				SendMessage(Format("  %s", i->c_str()));
		}
	}
	catch(const not_found_error &)
	{
		SendMessage("Error: squad not found.");
	}
}

void Player::ProcessBan(const std::string & name, const std::string & comment, bool netBan, bool banAllInRange, bool removeIdNumber, bool dontKick, bool banInPlayerZone, uint32 banDayCount, uint32 access, const InetAddress & ipAddress, uint32 ipMask, uint32 machineId)
{
	if(!Player::IsValidName(name))
	{
		SendMessage(Format("Error: invalid player name '%s'.", name.c_str()));
		return;
	}

	try
	{
		try
		{
			Player * target = PlayerManager::Instance().GetByName(name, GetZone().GetId());

			uint32 banId;
			Database::BanAdd(GetId(), GetZone().GetId(), target->GetId(), comment, netBan, banAllInRange, banDayCount, access, target->GetIpAddress(), (ipAddress != InetAddress() && ipAddress != target->GetIpAddress()) ? &ipAddress : 0, ipMask, (removeIdNumber ? 0 : target->GetMachineId()), (machineId != 0 && machineId != target->GetMachineId() && !removeIdNumber) ? &machineId : 0, target->GetBanData(), banId).Execute();

			target->BanKicked(!dontKick);
			SendMessage(Format("Ban #%d activated.", banId));
		}
		catch(const player_not_found_error &)
		{
			uint32 id;
			InetAddress ip;
			uint32 mid;
			uint32 timezonebias;
			BanData banData;
			Database::PlayerGetLastLoginInfo(name, netBan, GetZone().GetId(), id, ip, mid, timezonebias, banData).Execute();

			uint32 banId;
			Database::BanAdd(GetId(), GetZone().GetId(), id, comment, netBan, banAllInRange, banDayCount, access, ip, (ipAddress != InetAddress() && ipAddress != ip) ? &ipAddress : 0, ipMask, (removeIdNumber ? 0 : mid), (machineId != 0 && machineId != mid && !removeIdNumber) ? &machineId : 0, banData, banId).Execute();
			SendMessage(Format("Ban #%d activated.", banId));
		}
	}
	catch(const player_not_found_error &)
	{
		SendMessage(Format("Error: player '%s' not found.", name.c_str()));
	}
	catch(const operation_not_permitted_error &)
	{
		SendMessage(Format("Error: player '%s' is an operator.", name.c_str()));
	}
	catch(const permission_error &)
	{
		SendMessage("Error: you do not have permission to place the specified ban.");
	}
}

void Player::ProcessChangeBan(uint32 banId, bool * banAllInRange, bool * removeIdNumber, uint32 * banDayCount, uint32 * access, const InetAddress * ipAddress, uint32 * ipMask, uint32 * machineId)
{
	try
	{
		uint32 noMachineId = 0;

		Database::BanChange(GetId(), GetZone().GetId(), banId, banAllInRange, banDayCount, access, (ipAddress != 0 && *ipAddress != InetAddress()) ? ipAddress : 0, ipMask, (removeIdNumber != 0 && *removeIdNumber) ? &noMachineId : ((machineId != 0 && *machineId != 0) ? machineId : 0)).Execute();
		SendMessage(Format("Ban #%d changed.", banId));
	}
	catch(const ban_not_found_error &)
	{
		SendMessage(Format("Error: ban #%d not found.", banId));
	}
	catch(const permission_error &)
	{
		SendMessage(Format("Error: you do not have permission to change ban #%d.", banId));
	}

}

void Player::ProcessLiftBan(uint32 banId)
{
	try
	{
		Database::BanLift(GetId(), GetZone().GetId(), banId).Execute();
		SendMessage(Format("Ban #%d lifted.", banId));
	}
	catch(const ban_not_found_error &)
	{
		SendMessage(Format("Error: ban #%d not found.", banId));
	}
	catch(const permission_error &)
	{
		SendMessage(Format("Error: you do not have permission to lift ban #%d.", banId));
	}
}

void Player::ProcessListBan(bool netban, bool details, uint32 limit)
{
	try
	{
		Database::BanList list(GetId(), GetZone().GetId(), netban, limit);

		list.Execute();
		if(list.empty())
			SendMessage("There are no bans.");
		else
			for(Database::BanList::iterator i = list.begin(); i != list.end(); ++i)
			{
				SendMessage(Format("%c%-5d:%d by %-10.10s %s days:%-4d %-18.18s %s", (i->iIsActive) ? '#' : '^', i->iId, i->iAccess, i->iSysop.c_str(), i->iBanDate.c_str(), i->iBanDays, i->iIpRange.c_str(), i->iPlayerName.c_str()));
				if(details && !i->iComment.empty())
					SendMessage("     " + i->iComment);
			}
	}
	catch(const permission_error &)
	{
		SendMessage("Error: you do not have permission to list bans.");
	}
}

void Player::ProcessBanComment(uint32 banId, const std::string & comment)
{
	try
	{
		Database::BanAddComment(GetId(), GetZone().GetId(), banId, comment).Execute();
		SendMessage(Format("Added comment to ban #%d.", banId));
	}
	catch(const ban_not_found_error &)
	{
		SendMessage(Format("Error: ban #%d not found.", banId));
	}
	catch(const permission_error &)
	{
		SendMessage(Format("Error: you do not have permission to modify ban #%d.", banId));
	}
}

void Player::ProcessBanFree(const std::string & name, uint32 access, bool netban)
{
	if(!Player::IsValidName(name))
	{
		SendMessage(Format("Error: invalid player name '%s'.", name.c_str()));
		return;
	}

	try
	{
		Database::BanFreeAdd(GetId(), netban, GetZone().GetId(), access, name).Execute();
		SendMessage(Format("Ban free player added: %s", name.c_str()));
	}
	catch(const player_not_found_error &)
	{
		SendMessage(Format("Error: player '%s' not found.", name.c_str()));
	}
	catch(const permission_error &)
	{
		SendMessage(Format("Error: you do not have permission to ban free '%s'.", name.c_str()));
	}
	catch(const constraint_error &)
	{
		SendMessage(Format("Error: player '%s' is already on the ban free list.", name.c_str()));
	}
}

void Player::ProcessRemoveBanFree(const std::string & name, bool netban)
{
	if(!Player::IsValidName(name))
	{
		SendMessage(Format("Error: invalid player name '%s'.", name.c_str()));
		return;
	}

	try
	{
		Database::BanFreeRemove(GetId(), netban, GetZone().GetId(), name).Execute();
		SendMessage(Format("Removed ban free player: %s", name.c_str()));
	}
	catch(const player_not_found_error &)
	{
		SendMessage(Format("Error: player '%s' not found or is not on the ban free list.", name.c_str()));
	}
	catch(const permission_error &)
	{
		SendMessage(Format("Error: you do not have permission to remove ban free from '%s'.", name.c_str()));
	}
}

void Player::ProcessListBanFree(bool netban, uint32 limit)
{
	try
	{
		Database::BanFreeList list(GetId(), GetZone().GetId(), netban, limit);

		list.Execute();
		if(list.empty())
			SendMessage("There are no ban frees.");
		else
			for(Database::BanFreeList::iterator i = list.begin(); i != list.end(); ++i)
				SendMessage(Format("%d:%-20.20s %s %-20.20s", i->iAccess, i->iGranter.c_str(), i->iTime.c_str(), i->iName.c_str()));
	}
	catch(const permission_error &)
	{
		SendMessage("Error: you do not have permission to view the ban free list.");
	}
}

void Player::ProcessAddOp(uint32 access, bool netOp, const std::string & name)
{
	if(!Player::IsValidName(name))
	{
		SendMessage(Format("Error: invalid player name '%s'.", name.c_str()));
		return;
	}

	//
	// Level 0 BanG is always a netop, levels > 1 are never netops,
	// level = 1 could be either.
	//
	if(access == 0)
		netOp = true;
	if(access > 1)
		netOp = false;

	try
	{
		std::string opGroup = Format("BanG L%d", access);

		//
		// BanG netops are always global ops.
		//
		if(netOp)
			Database::BanAddGlobalSysop(GetId(), GetZone().GetId(), name, opGroup + "N").Execute();
		else
			Database::BanAddLocalSysop(GetId(), GetZone().GetId(), name, opGroup).Execute();

		SendMessage(Format("Added L%d operator '%s'.", access, name.c_str()));
	}
	catch(const player_not_found_error &)
	{
		SendMessage(Format("Error: could not find player '%s'.", name.c_str()));
	}
	catch(const op_group_not_found_error &)
	{
		SendMessage("Error: could not find operator group for given access level and type.");
	}
	catch(const constraint_error &)
	{
		SendMessage(Format("Error: player '%s' is already an operator.", name.c_str()));
	}
	catch(const permission_error &)
	{
		SendMessage("Error: you do not have permission to add this type of operator.");
	}
}

void Player::ProcessRemoveOp(const std::string & name, bool netOp)
{
	if(!Player::IsValidName(name))
	{
		SendMessage(Format("Error: invalid player name '%s'.", name.c_str()));
		return;
	}

	try
	{
		if(netOp)
			Database::BanRemoveGlobalSysop(GetId(), GetZone().GetId(), name).Execute();
		else
			Database::BanRemoveLocalSysop(GetId(), GetZone().GetId(), name).Execute();

		SendMessage(Format("Removed operator '%s'.", name.c_str()));
	}
	catch(const player_not_found_error &)
	{
		SendMessage(Format("Error: could not find player '%s'.", name.c_str()));
	}
	catch(const op_not_found_error &)
	{
		SendMessage(Format("'%s' does not have billing operator powers in this zone.", name.c_str()));
	}
	catch(const permission_error &)
	{
		SendMessage(Format("Error: you do not have permission to remove '%s'.", name.c_str()));
	}
}

void Player::ProcessListOp(bool netwide)
{
	try
	{
		Database::BanListSysop list(GetId(), GetZone().GetId(), netwide);
		list.Execute();

		if(list.empty())
			SendMessage("There are no operators.");
		else
			for(Database::BanListSysop::iterator i = list.begin(); i != list.end(); ++i)
				try
				{
					PlayerManager::Instance().GetByName(i->iName, GetZone().GetId());
					SendMessage(Format("%d:%-24.24s (logged in)", i->iAccess, i->iName.c_str()));
				}
				catch(const not_found_error &)
				{
					SendMessage(Format("%d:%s", i->iAccess, i->iName.c_str()));
				}
	}
	catch(const permission_error &)
	{
		SendMessage("Error: you do not have permission to list operators.");
	}
}

void Player::ProcessMessage(const std::string & player, const std::string & message)
{
	try
	{
		Database::MessageSend msg(GetId(), player, message);
		msg.Execute();
		if(msg.Replaced())
			SendMessage("Message replaced.");
		else
			SendMessage("Message stored.");

		Player * target = PlayerManager::Instance().GetByName(player);
		target->SendMessage(Format("You have received a new message from %s. Type ?messages to read the message.", GetName().c_str()));
	}
	catch(const not_found_error &)
	{
		SendMessage(Format("Error: could not find player '%s'.", player.c_str()));
	}
}

void Player::ProcessMessages()
{
	Database::MessageReadAll msg(GetId());
	msg.Execute();
	if(msg.empty())
		SendMessage("You have no stored messages.");
	else
		for(Database::MessageReadAll::iterator i = msg.begin(); i != msg.end(); ++i)
			SendMessage(Format("[%s] %s: %s", i->iTime.c_str(), i->iFrom.c_str(), i->iMessage.c_str()));

	iReadMessages = true;
}

void Player::ProcessSetPassword(const std::string & newPassword)
{
	Database::PlayerUpdatePassword(GetId(), newPassword).Execute();
	SendMessage("Password successfully updated.");
}

void Player::ProcessAlias(const std::string & name)
{
	if(!Player::IsValidName(name))
	{
		SendMessage(Format("Error: invalid player name %s.", name.c_str()));
		return;
	}

	try
	{
		Database::BanAlias aliases(GetId(), GetZone().GetId(), name);
		aliases.Execute();

		if(aliases.empty())
			SendMessage(Format("No aliases found for %s.", name.c_str()));
		else
			for(Database::BanAlias::iterator i = aliases.begin(); i != aliases.end(); ++i)
			{
				std::string message = i->second.iName;

				if(!i->second.iIpAddress.empty())
					message += ":ip=" + i->second.iIpAddress;

				if(i->second.iMachineId != 0)
					message += Format(":mid=%d", i->second.iMachineId);

				if(i->second.iAdditionalMatches != 0)
					message += Format(" (%d additional matches)", i->second.iAdditionalMatches);

				SendMessage(message);
			}
	}
	catch(const permission_error &)
	{
		SendMessage("You do not have permission to check aliases.");
	}
	catch(const not_found_error &)
	{
		SendMessage(Format("Player %s not found.", name.c_str()));
	}
}

void Player::ProcessZone()
{
	SendMessage(iZone.GetName());
}

void Player::ProcessZones()
{
	ZoneManager & zones = ZoneManager::Instance();
	for(ZoneManager::iterator i = zones.begin(); i != zones.end(); ++i)
		SendMessage(Format("%-32s (%d)", i->second->GetName().substr(0, 31).c_str(), i->second->GetPlayerCount()));
}

void Player::ProcessFind(const std::string & name)
{
	if(name.empty())
		return;

	try
	{
		Player * player = PlayerManager::Instance().GetByName(name);
		SendMessage(Format("%s is in %s", player->GetName().c_str(), player->GetZone().GetName().c_str()));
	}
	catch(const not_found_error &)
	{
		try
		{
			std::string lastSeen;
			Database::PlayerGetLastSeen(name, lastSeen).Execute();

			uint32 days = 0, hours = 0, minutes = 0, seconds = 0;
			if(sscanf(lastSeen.c_str(), "%d %*s %d:%d:%d", &days, &hours, &minutes, &seconds) != 4)
			{
				days = hours = minutes = seconds = 0;
				sscanf(lastSeen.c_str(), "%d:%d:%d", &hours, &minutes, &seconds);
			}

			if(days * 24 + hours >= 48)
				SendMessage(Format("Player is not online, last seen %d days ago.", (days * 24 + hours) / 24));
			else if(days > 0 || hours > 0)
				SendMessage(Format("Player is not online, last seen %d %s, %d %s ago.", days * 24 + hours, (days * 24 + hours == 1 ? "hour" : "hours"), minutes, (minutes == 1) ? "minute" : "minutes"));
			else
				SendMessage(Format("Player is not online, last seen %d %s ago.", minutes, (minutes == 1) ? "minute" : "minutes"));
		}
		catch(const not_found_error &)
		{
			SendMessage("Unknown player name.");
		}
	}
}

void Player::ProcessVersion()
{
	SendMessage(VERSION);
}

void Player::ProcessTop10Points()
{
	Database::ScoreGetTop10Points top(GetZone().GetScoreId());
	top.Execute();

	SendMessage("Top 10 players by points:");
	uint32 ordinal = 1;
	for(Database::ScoreGetTop10Points::iterator i = top.begin(); i != top.end(); ++i, ++ordinal)
		SendMessage(Format("%2d. %-24.24s  %d", ordinal, i->iName.c_str(), i->iPoints));
}

void Player::ProcessTop10Kills()
{
	Database::ScoreGetTop10Kills top(GetZone().GetScoreId());
	top.Execute();

	SendMessage("Top 10 players by kills:");
	uint32 ordinal = 1;
	for(Database::ScoreGetTop10Kills::iterator i = top.begin(); i != top.end(); ++i, ++ordinal)
		SendMessage(Format("%2d. %-24.24s  %d", ordinal, i->iName.c_str(), i->iKills));
}

void Player::ProcessTop10Deaths()
{
	Database::ScoreGetTop10Deaths top(GetZone().GetScoreId());
	top.Execute();

	SendMessage("Top 10 players by deaths:");
	uint32 ordinal = 1;
	for(Database::ScoreGetTop10Deaths::iterator i = top.begin(); i != top.end(); ++i, ++ordinal)
		SendMessage(Format("%2d. %-24.24s  %d", ordinal, i->iName.c_str(), i->iDeaths));
}

void Player::ProcessTop10SquadPoints()
{
	Database::ScoreGetTop10SquadPoints top(GetZone().GetScoreId());
	top.Execute();

	SendMessage("Top 10 squads by points:");
	uint32 ordinal = 1;
	for(Database::ScoreGetTop10SquadPoints::iterator i = top.begin(); i != top.end(); ++i, ++ordinal)
		SendMessage(Format("%2d. %-24.24s  %d", ordinal, i->iName.c_str(), i->iPoints));
}

void Player::ProcessTop10SquadKills()
{
	Database::ScoreGetTop10SquadKills top(GetZone().GetScoreId());
	top.Execute();

	SendMessage("Top 10 squads by kills:");
	uint32 ordinal = 1;
	for(Database::ScoreGetTop10SquadKills::iterator i = top.begin(); i != top.end(); ++i, ++ordinal)
		SendMessage(Format("%2d. %-24.24s  %d", ordinal, i->iName.c_str(), i->iKills));
}

void Player::ProcessTop10SquadDeaths()
{
	Database::ScoreGetTop10SquadDeaths top(GetZone().GetScoreId());
	top.Execute();

	SendMessage("Top 10 squads by deaths:");
	uint32 ordinal = 1;
	for(Database::ScoreGetTop10SquadDeaths::iterator i = top.begin(); i != top.end(); ++i, ++ordinal)
		SendMessage(Format("%2d. %-24.24s  %d", ordinal, i->iName.c_str(), i->iDeaths));
}

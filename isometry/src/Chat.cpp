#include "base.h"
#include "Chat.h"
#include "Player.h"
#include "Database.h"
#include "ChatManager.h"
#include "PlayerManager.h"

Chat::Chat(uint32 chatId)
	: iId(chatId)
{
	Database::ChatGetInfo(iId, iName, iOwnerId, iOwnerName, iIsPublic).Execute();
	Database::ChatGetMemberList list(GetId());
	list.Execute();
	for(Database::ChatGetMemberList::iterator i = list.begin(); i != list.end(); ++i)
		iMembers.insert(*i);
}

Chat::Chat(const std::string & name)
	: iName(name)
{
	Database::ChatGetInfo(iId, iName, iOwnerId, iOwnerName, iIsPublic).Execute();
	Database::ChatGetMemberList list(GetId());
	list.Execute();
	for(Database::ChatGetMemberList::iterator i = list.begin(); i != list.end(); ++i)
		iMembers.insert(*i);
}

Chat::~Chat()
{
}

bool Chat::IsValidName(const std::string & name)
{
	return (!name.empty() && name.length() <= 24 && name.find_first_of(":,") == std::string::npos);
}

uint32 Chat::GetId() const
{
	return iId;
}

const std::string & Chat::GetName() const
{
	return iName;
}

const std::string & Chat::GetOwnerName() const
{
	return iOwnerName;
}

uint32 Chat::GetOwnerId() const
{
	return iOwnerId;
}

bool Chat::IsPublic() const
{
	return iIsPublic;
}

const Chat::MemberList & Chat::GetMembers() const
{
	return iMembers;
}

const Chat::ActiveMemberList & Chat::GetActiveMembers() const
{
	return iActiveMembers;
}

void Chat::SendMessage(const Player & from, const std::string & message)
{
	for(ActiveMemberList::iterator i = iActiveMembers.begin(); i != iActiveMembers.end(); ++i)
		if(*i != &from)
			(*i)->SendChatMessage(*this, Format("%s> %s", from.GetName().c_str(), message.c_str()));
}

void Chat::Join(Player & player, uint32 ordinal)
{
	Database::ChatJoin(GetId(), IsPublic(), GetOwnerId(), player.GetId(), ordinal).Execute();
	iMembers.insert(player.GetName());

	Login(player);
}

void Chat::Leave(Player & player)
{
	Database::ChatLeave(GetId(), player.GetId()).Execute();
	iMembers.erase(player.GetName());

	Logout(player);
}

void Chat::Grant(const Player & granter, const std::string & player)
{
	//
	// The owner might already be in the allow list so ignore the error
	// if that's the case.
	//
	try
	{
		Allow(granter, granter.GetName());
	}
	catch(const constraint_error &)
	{ }

	Database::ChatGrant(GetId(), GetOwnerId(), granter.GetId(), player, iOwnerId).Execute();
	iOwnerName = player;
}

void Chat::SetPublic(const Player & requester, bool isPublic)
{
	Database::ChatSetPublic(GetId(), requester.GetId(), isPublic).Execute();
	iIsPublic = isPublic;
}

void Chat::Allow(const Player & granter, const std::string & player)
{
	Database::ChatAllow(GetId(), GetOwnerId(), granter.GetId(), player).Execute();
}

void Chat::Deny(const Player & denyer, const std::string & player)
{
	Database::ChatDeny(GetId(), GetOwnerId(), denyer.GetId(), player).Execute();
	iMembers.erase(player);

	try
	{
		Player * p = PlayerManager::Instance().GetByName(player);
		p->ChatKicked(*this);
		Logout(*p);
	}
	catch(const not_found_error &)
	{ }
}

void Chat::Login(Player & player)
{
	iActiveMembers.insert(&player);
}

void Chat::Logout(Player & player)
{
	iActiveMembers.erase(&player);
}

void Chat::Delete(const Player & deleter)
{
	if(deleter.GetId() != GetOwnerId())
		throw permission_error("[Chat::Delete]: only owner can delete the chat.");

	Database::ChatDelete(GetId(), deleter.GetId()).Execute();

	for(ActiveMemberList::iterator i = iActiveMembers.begin(); i != iActiveMembers.end(); ++i)
		if(*i != &deleter)
			(*i)->ChatDeleted(*this);

	ChatManager::Instance().Delete(*this);
}

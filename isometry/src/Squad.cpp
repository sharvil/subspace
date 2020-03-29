#include "base.h"
#include "Squad.h"
#include "Player.h"
#include "Database.h"
#include "SquadManager.h"
#include "PlayerManager.h"

Squad::Squad(uint32 squadId)
	: iId(squadId)
{
	Database::SquadGetInfo(iId, iName, iOwnerId, iOwnerName, iIsPublic).Execute();
	Database::SquadGetMemberList list(GetId());
	list.Execute();
	for(Database::SquadGetMemberList::iterator i = list.begin(); i != list.end(); ++i)
		iMembers.insert(*i);
}

Squad::Squad(const std::string & name)
	: iName(name)
{
	Database::SquadGetInfo(iId, iName, iOwnerId, iOwnerName, iIsPublic).Execute();
	Database::SquadGetMemberList list(GetId());
	list.Execute();
	for(Database::SquadGetMemberList::iterator i = list.begin(); i != list.end(); ++i)
		iMembers.insert(*i);
}

Squad::~Squad()
{
}

bool Squad::IsValidName(const std::string & name)
{
	return (!name.empty() && name.length() <= 24 && name.find(':') == std::string::npos);
}

uint32 Squad::GetId() const
{
	return iId;
}

const std::string & Squad::GetName() const
{
	return iName;
}

const std::string & Squad::GetOwnerName() const
{
	return iOwnerName;
}

uint32 Squad::GetOwnerId() const
{
	return iOwnerId;
}

bool Squad::IsPublic() const
{
	return iIsPublic;
}

const Squad::MemberList & Squad::GetMembers() const
{
	return iMembers;
}

void Squad::SendMessage(const std::string & message)
{
	for(ActiveMemberList::iterator i = iActiveMembers.begin(); i != iActiveMembers.end(); ++i)
		(*i)->SendRemotePlayerMessage(message);
}

void Squad::Join(Player & player)
{
	Database::SquadJoin(GetId(), IsPublic(), GetOwnerId(), player.GetId()).Execute();
	iMembers.insert(player.GetName());

	Login(player);
}

void Squad::Leave(Player & player)
{
	Database::SquadLeave(GetId(), player.GetId()).Execute();
	iMembers.erase(player.GetName());

	Logout(player);
}

void Squad::Grant(const Player & granter, const std::string & player)
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

	Database::SquadGrant(GetId(), GetOwnerId(), granter.GetId(), player, iOwnerId).Execute();
	iOwnerName = player;
}

void Squad::SetPublic(const Player & requester, bool isPublic)
{
	Database::SquadSetPublic(GetId(), requester.GetId(), isPublic).Execute();
	iIsPublic = isPublic;
}

void Squad::Allow(const Player & granter, const std::string & player)
{
	Database::SquadAllow(GetId(), GetOwnerId(), granter.GetId(), player).Execute();
}

void Squad::Deny(const Player & denyer, const std::string & player)
{
	Database::SquadDeny(GetId(), GetOwnerId(), denyer.GetId(), player).Execute();
	iMembers.erase(player);

	try
	{
		Player * p = PlayerManager::Instance().GetByName(player);
		p->SquadKicked();
		Logout(*p);
	}
	catch(const not_found_error &)
	{ }
}

void Squad::Login(Player & player)
{
	DataWriter <LittleEndian, AsciiZ> writer;
	writer.AppendUint8(0x85);
	writer.AppendString(player.GetName());

	for(ActiveMemberList::iterator i = iActiveMembers.begin(); i != iActiveMembers.end(); ++i)
		if((*i)->GetName() != player.GetName())
			(*i)->SendRawPacket(writer.Asbstring());

	iActiveMembers.insert(&player);
}

void Squad::Logout(Player & player)
{
	iActiveMembers.erase(&player);

	DataWriter <LittleEndian, AsciiZ> writer;
	writer.AppendUint8(0x86);
	writer.AppendString(player.GetName());

	for(ActiveMemberList::iterator i = iActiveMembers.begin(); i != iActiveMembers.end(); ++i)
		if((*i)->GetName() != player.GetName())
			(*i)->SendRawPacket(writer.Asbstring());
}

void Squad::Delete(const Player & deleter)
{
	if(deleter.GetId() != GetOwnerId())
		throw permission_error("[Squad::Delete]: only owner can delete the squad.");

	Database::SquadDelete(GetId(), deleter.GetId()).Execute();

	for(ActiveMemberList::iterator i = iActiveMembers.begin(); i != iActiveMembers.end(); ++i)
		if(*i != &deleter)
			(*i)->SquadDeleted();

	SquadManager::Instance().Delete(*this);
}

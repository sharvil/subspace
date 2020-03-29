#include "base.h"
#include "PlayerManager.h"
#include "Zone.h"
#include "Exceptions.h"

PlayerManager & PlayerManager::Instance()
{
	static PlayerManager self;
	return self;
}

Player * PlayerManager::GetByName(const std::string & name)
{
	PlayerMap::iterator i = iPlayers.find(name);
	if(i == iPlayers.end())
		throw player_not_found_error("player");

	return i->second.front();
}

Player * PlayerManager::GetByName(const std::string & name, uint32 zoneId)
{
	PlayerMap::iterator i = iPlayers.find(name);
	if(i == iPlayers.end())
		throw player_not_found_error("player");

	for(PlayerLogins::iterator j = i->second.begin(); j != i->second.end(); ++j)
		if((*j)->GetZone().GetId() == zoneId)
			return *j;

	throw player_not_found_error("player");
}

void PlayerManager::Register(Player & player)
{
	PlayerMap::iterator i = iPlayers.find(player.GetName());
	if(i == iPlayers.end())
	{
		PlayerLogins l;
		l.push_back(&player);
		iPlayers.insert(std::make_pair(player.GetName(), l));
	}
	else
	{
		//
		// Make sure we don't duplicate the pointer in the login vector.
		//
		for(PlayerLogins::iterator j = i->second.begin(); j != i->second.end(); ++j)
			if(*j == &player)
				return;

		i->second.push_back(&player);
	}
}

void PlayerManager::Unregister(const Player & player)
{
	PlayerMap::iterator i = iPlayers.find(player.GetName());
	if(i == iPlayers.end())
		return;

	for(PlayerLogins::iterator j = i->second.begin(); j != i->second.end(); ++j)
		if(*j == &player)
		{
			i->second.erase(j);
			break;
		}

	if(i->second.empty())
		iPlayers.erase(i);
}

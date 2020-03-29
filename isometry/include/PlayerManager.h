#pragma once

#include "base.h"
#include "Player.h"

#include <map>
#include <vector>

class PlayerManager : public NonCopyable
{
	public:
		friend class Player;
		static PlayerManager & Instance();

		Player * GetByName(const std::string & name);
		Player * GetByName(const std::string & name, uint32 zoneId);

	private:
		typedef std::vector <Player *> PlayerLogins;
		typedef std::map <std::string, PlayerLogins, String::CaseInsensitiveComparator> PlayerMap;

		void Register(Player & player);
		void Unregister(const Player & player);

		PlayerMap iPlayers;
};

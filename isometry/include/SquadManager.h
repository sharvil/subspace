#pragma once

#include "base.h"
#include "Squad.h"

#include <map>

class SquadManager : public NonCopyable
{
	public:
		friend class Squad;
		static SquadManager & Instance();

		Squad * GetById(uint32 squadId);
		Squad * GetByName(const std::string & name);

	private:
		typedef std::map <uint32, Squad *> SquadMap;

		~SquadManager();
		void Delete(Squad & squad);

		SquadMap iSquads;
};

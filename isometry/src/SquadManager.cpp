#include "base.h"
#include "SquadManager.h"

SquadManager & SquadManager::Instance()
{
	static SquadManager self;
	return self;
}

SquadManager::~SquadManager()
{
	for(SquadMap::iterator i = iSquads.begin(); i != iSquads.end(); ++i)
		delete i->second;
}

Squad * SquadManager::GetById(uint32 squadId)
{
	SquadMap::iterator i = iSquads.find(squadId);
	if(i != iSquads.end())
		return i->second;

	Squad * squad = new Squad(squadId);
	iSquads.insert(std::make_pair(squadId, squad));
	return squad;
}

Squad * SquadManager::GetByName(const std::string & name)
{
	for(SquadMap::iterator i = iSquads.begin(); i != iSquads.end(); ++i)
		if(String::Compare(i->second->GetName(), name, String::KCaseInsensitive) == 0)
			return i->second;

	Squad * squad = new Squad(name);
	iSquads.insert(std::make_pair(squad->GetId(), squad));
	return squad;
}

void SquadManager::Delete(Squad & squad)
{
	iSquads.erase(squad.GetId());
	delete &squad;
}

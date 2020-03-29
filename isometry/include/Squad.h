#pragma once

#include "base.h"
#include "Player.h"

#include <set>

class Squad : public NonCopyable
{
	public:
		friend class SquadManager;
		typedef std::set <Player *> ActiveMemberList;
		typedef std::set <std::string> MemberList;

		static bool IsValidName(const std::string & name);

		uint32 GetId() const;
		const std::string & GetName() const;
		const std::string & GetOwnerName() const;
		uint32 GetOwnerId() const;
		bool IsPublic() const;
		const MemberList & GetMembers() const;

		void SendMessage(const std::string & message);

		void Grant(const Player & granter, const std::string & player);
		void SetPublic(const Player & requester, bool isPublic);

		void Join(Player & player);
		void Leave(Player & player);

		void Allow(const Player & granter, const std::string & player);
		void Deny(const Player & denyer, const std::string & player);

		void Login(Player & player);
		void Logout(Player & player);

		void Delete(const Player & deleter);

	private:
		explicit Squad(uint32 squadId);
		explicit Squad(const std::string & name);
		~Squad();

		uint32 iId;
		std::string iName;
		std::string iOwnerName;
		uint32 iOwnerId;
		bool iIsPublic;
		MemberList iMembers;
		ActiveMemberList iActiveMembers;
};

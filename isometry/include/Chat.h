#pragma once

#include "base.h"
#include "Player.h"

#include <set>

class Chat : public NonCopyable
{
	public:
		friend class ChatManager;
		typedef std::set <Player *> ActiveMemberList;
		typedef std::set <std::string> MemberList;

		static bool IsValidName(const std::string & name);

		uint32 GetId() const;
		const std::string & GetName() const;
		const std::string & GetOwnerName() const;
		uint32 GetOwnerId() const;
		bool IsPublic() const;
		const MemberList & GetMembers() const;
		const ActiveMemberList & GetActiveMembers() const;

		void SendMessage(const Player & from, const std::string & message);

		void Grant(const Player & granter, const std::string & player);
		void SetPublic(const Player & requester, bool isPublic);

		void Allow(const Player & granter, const std::string & player);
		void Deny(const Player & denyer, const std::string & player);

		void Login(Player & player);
		void Logout(Player & player);

		void Join(Player & player, uint32 ordinal);
		void Leave(Player & player);

		void Delete(const Player & deleter);

	private:
		explicit Chat(uint32 chatId);
		explicit Chat(const std::string & name);
		~Chat();

		uint32 iId;
		std::string iName;
		std::string iOwnerName;
		uint32 iOwnerId;
		bool iIsPublic;
		MemberList iMembers;
		ActiveMemberList iActiveMembers;
};

inline bool operator == (const std::string & chatName, const Chat & chat)
{
	return (String::Compare(chatName, chat.GetName(), String::KCaseInsensitive) == 0);
}

inline bool operator != (const std::string & chatName, const Chat & chat)
{
	return !(chatName == chat);
}

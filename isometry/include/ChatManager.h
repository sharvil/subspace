#pragma once

#include "base.h"
#include "Squad.h"

#include <map>

class ChatManager : public NonCopyable
{
	public:
		friend class Chat;
		static ChatManager & Instance();

		Chat * GetById(uint32 chatId);
		Chat * GetByName(const std::string & name);

	private:
		typedef std::map <uint32, Chat *> ChatMap;

		~ChatManager();
		void Delete(Chat & chat);

		ChatMap iChats;
};

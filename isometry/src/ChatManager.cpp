#include "base.h"
#include "ChatManager.h"
#include "Chat.h"

ChatManager & ChatManager::Instance()
{
	static ChatManager self;
	return self;
}

ChatManager::~ChatManager()
{
	for(ChatMap::iterator i = iChats.begin(); i != iChats.end(); ++i)
		delete i->second;
}

Chat * ChatManager::GetById(uint32 chatId)
{
	ChatMap::iterator i = iChats.find(chatId);
	if(i != iChats.end())
		return i->second;

	Chat * chat = new Chat(chatId);
	iChats.insert(std::make_pair(chatId, chat));
	return chat;
}

Chat * ChatManager::GetByName(const std::string & name)
{
	for(ChatMap::iterator i = iChats.begin(); i != iChats.end(); ++i)
		if(String::Compare(i->second->GetName(), name, String::KCaseInsensitive) == 0)
			return i->second;

	Chat * chat = new Chat(name);
	iChats.insert(std::make_pair(chat->GetId(), chat));
	return chat;
}

void ChatManager::Delete(Chat & chat)
{
	iChats.erase(chat.GetId());
	delete &chat;
}

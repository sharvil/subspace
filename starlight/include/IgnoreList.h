#pragma once

#include "common.h"

#include <set>

class IgnoreList
{
	public:
		IgnoreList();

		bool IsIgnored(const std::string & name) const;

		void AddName(const std::string & name);
		void RemoveName(const std::string & name);

	private:
		typedef std::set <std::string, String::CaseInsensitiveComparator> container_t;
		static const std::string KIgnoreListFilename;

		void LoadFile();
		void SaveFile() const;

		container_t iNames;
};

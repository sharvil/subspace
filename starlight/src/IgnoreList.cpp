#include "common.h"
#include "IgnoreList.h"

const std::string IgnoreList::KIgnoreListFilename("data/ignore.txt");

IgnoreList::IgnoreList()
{
	LoadFile();
}

bool IgnoreList::IsIgnored(const std::string & name) const
{
	return iNames.find(name) != iNames.end();
}

void IgnoreList::AddName(const std::string & name)
{
	iNames.insert(name);
	SaveFile();
}

void IgnoreList::RemoveName(const std::string & name)
{
	iNames.erase(name);
	SaveFile();
}

void IgnoreList::LoadFile()
{
	try
	{
		uint32 length = File::Length(KIgnoreListFilename);
		std::string contents(length, 0);

		File fp(KIgnoreListFilename, "rb");
		fp.Read(&contents[0], length);
		fp.Close();

		iNames.clear();
		StringSequence names = String::Split(contents, '\n');
		for(StringSequence::iterator i = names.begin(); i != names.end(); ++i)
		{
			String::Trim(*i);
			if(!i->empty())
				iNames.insert(*i);
		}
	}
	catch(const not_found_error &)
	{
		//
		// It's okay if there's no ignore list - we'll create one if need be.
		//
	}
}

void IgnoreList::SaveFile() const
{
	File fp(KIgnoreListFilename, "wt");
	for(container_t::const_iterator i = iNames.begin(); i != iNames.end(); ++i)
	{
		fp.Write(i->c_str(), i->length());
		fp.Write("\n", 1);
	}
}

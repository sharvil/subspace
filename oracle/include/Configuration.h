/*++

Copyright (c) Sharvil Nanavati.

Module:

    Configuration.h

Authors:

    Sharvil Nanavati (sharvil) 2006-08-09

--*/

#pragma once

#include "base.h"

class Configuration
{
	public:
		Configuration();

		void LoadFromArguments(const_cstring * arguments, uint32 count);
		void Load(const std::string & filename);
		void Reset();

		std::string GetExeName() const;
		std::string GetLogName() const;
		uint16 GetListenPort() const;

	private:
		std::string iExeName;
		std::string iLogFilename;
		uint16 iListenPort;
};

/*++

Copyright (c) Sharvil Nanavati.

Module:

    Configuration.cpp

Authors:

    Sharvil Nanavati (sharvil) 2006-08-09

--*/

#include "base.h"
#include "Nucleus.h"
#include "Configuration.h"

Configuration::Configuration()
	: iListenPort(0)
{
}

void Configuration::LoadFromArguments(const_cstring * arguments, uint32 count)
{
	iExeName = "Continuum.exe";
	iListenPort = 6000;
}

void Configuration::Load(const std::string & filename)
{
}

void Configuration::Reset()
{
	iLogFilename.clear();
	iListenPort = 0;
}

std::string Configuration::GetExeName() const
{
	return iExeName;
}

std::string Configuration::GetLogName() const
{
	return iLogFilename;
}

uint16 Configuration::GetListenPort() const
{
	return iListenPort;
}

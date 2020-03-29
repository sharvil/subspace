/*++

Copyright (c) Sharvil Nanavati.

Module:

    main.cpp

Authors:

    Sharvil Nanavati (sharvil) 2008-04-27

--*/

#include "base.h"
#include "SSNetLib.h"
#include "TimeLib.h"
#include "LogLib.h"
#include "DirectoryServer.h"
#include "Config.h"
#include "ZoneList.h"

#include <cstring>
#include <algorithm>

#if !defined(PLATFORM_WIN32)
#	include <signal.h>
#endif

static DirectoryServer * gsServer = NULL;

#if defined(PLATFORM_WIN32)
	static BOOL WINAPI quitHandler(DWORD type)
	{
		if(gsServer != 0)
			gsServer->Exit();
		return TRUE;
	}

	static void setSignalHandler()
	{
		SetConsoleTitle("SubSpace Directory Server");
		SetConsoleCtrlHandler(quitHandler, true);
	}
#else
	static void quitHandler(int32)
	{
		if(gsServer != 0)
			gsServer->Exit();
	}

	static void setSignalHandler()
	{
		struct sigaction quitSignal;
		memset(&quitSignal, 0, sizeof(quitSignal));
		quitSignal.sa_handler = quitHandler;
		if(sigaction(SIGINT, &quitSignal, 0) < 0)
			throw std::runtime_error("Unable to set signal handler.");
	}
#endif

nativeint main$(nativeint argc, const_cstring * argv)
{
	setSignalHandler();

	try
	{
		DirectoryServer server;
		gsServer = &server;
		server.Run();
	}
	catch(const std::runtime_error & err)
	{
		Logger::Instance().Log(KLogError, "%s", err.what());
	}

	return 0;
}

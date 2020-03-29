#include "base.h"
#include "SSNetLib.h"
#include "LogLib.h"
#include "DbLib.h"

#include "ZoneManager.h"
#include "Zone.h"

void usage()
{
	printf("Usage: %s [-p port]\n", Process::GetName().c_str());
	exit(1);
}

int32 main$(int32 argc, const_cstring * argv)
{
	uint16 port = 6006;
	for(int32 i = 1; i < argc; ++i)
	{
		std::string arg = argv[i];
		if(arg == "-p" && i + 1 < argc)
			if(sscanf(argv[++i], "%hd", &port) != 1 || port == 0)
				usage();
	}

	StdoutLogSink sink;
	Logger::Instance().AttachSink(sink);

	Selector selector;
	SSConnectionPool pool(selector);

	pool.Bind(port);
	pool.AddListener(ZoneManager::Instance());

	DbConnectionManager::Instance().Initialize("dbname=subspace user=subspace password=<redacted>");

	for(;;)
		selector.Select(100);

	return 0;
}

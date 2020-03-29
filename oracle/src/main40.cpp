/*++

Copyright (c) Sharvil Nanavati.

Module:

    main.cpp

Authors:

    Sharvil Nanavati (sharvil) 2006-08-06

--*/

#include "base.h"
#include "NetLib.h"
#include "LogLib.h"
#include "Nucleus.h"
#include "TimeLib.h"
#include "PeLoader.h"
#include "Configuration.h"

#include <set>
#include <iostream>
#include <windows.h>

PeLoader loader;

static void InitializeContinuum()
{
	//
	// Set up initial environment for Continuum executable. It needs a
	// private heap and a TLS area for its exception mechanism.
	//
	*(HANDLE *)loader.RVA(0x0C326C) = HeapCreate(0, 0, 0);
	*(DWORD *)loader.RVA(0x0B9F40) = TlsAlloc();

	//
	// The Continuum packer overwrites the import table when loading the
	// executable. The game then (accidentally) uses some of the overwritten
	// import table values for the EXE checksum. The following overwrites the
	// relevant parts of the import table.
	//
	for(uint32 * ptr = (uint32 *)loader.RVA(0x0B3E90); ptr <= (uint32 *)loader.RVA(0x0B3E9C); ++ptr)
		*ptr = (uint32)loader.RVA(0x0B4224);
}

static void GenerateKeystream(void * keystream, uint32 key1, uint32 key2)
{
	void ** ksptr = &keystream;
	void * base = loader.RVA(0);

	asm(
		"movl %0, %%ecx\n"
		"pushl %1\n"
		"movl $0x057D60, %%eax\n"
		"addl %3, %%eax\n"
		"call *%%eax\n"

		"movl %0, %%ecx\n"
		"pushl %1\n"
		"movl $0x058BE0, %%eax\n"
		"addl %3, %%eax\n"
		"call *%%eax\n"

		"movl %0, %%ecx\n"
		"pushl %2\n"
		"movl $0x058E90, %%eax\n"
		"addl %3, %%eax\n"
		"call *%%eax\n"

		:
		: "m" (ksptr), "m" (key2), "m" (key1), "m" (base)
		: "eax", "ecx"
	);
}

static void GenerateHalfstream(void * halfstream, uint32 key)
{
	void ** ksptr = &halfstream;
	void * base = loader.RVA(0);

	asm(
		"movl %0, %%ecx\n"
		"pushl %1\n"
		"movl $0x057D60, %%eax\n"
		"addl %2, %%eax\n"
		"call *%%eax\n"

		:
		: "m" (ksptr), "m" (key), "m" (base)
		: "eax", "ecx"
	);
}

static void GenerateStep12(void * keystream, uint32 key2)
{
	void ** ksptr = &keystream;
	void * base = loader.RVA(0);

	asm(
		"movl %0, %%ecx\n"
		"pushl %1\n"
		"movl $0x057D60, %%eax\n"
		"addl %2, %%eax\n"
		"call *%%eax\n"

		"movl %0, %%ecx\n"
		"pushl %1\n"
		"movl $0x058BE0, %%eax\n"
		"addl %2, %%eax\n"
		"call *%%eax\n"

		:
		: "m" (ksptr), "m" (key2), "m" (base)
		: "eax", "ecx"
	);
}

static uint32 GenerateExeChecksum(uint32 seed)
{
	asm(
		"pushl %1\n"
		"call *%2\n"
		"movl %%eax, %0\n"
		: "=m" (seed)
		: "m" (seed), "r" (loader.RVA(0x04DAC0))
		: "eax"
	);

	return seed;
}

static bstring KeystreamRequest(const InetAddress & source, const bstring & packet)
{
	if(packet.GetSize() != 12)
		return bstring();

	uint32 key1 = ((uint32 *)(packet.Data() + 2))[0];
	uint32 key2 = ((uint32 *)(packet.Data() + 2))[1];

	uint8 keystream[80];
	GenerateKeystream(keystream, key1, key2);

	Logger::Instance().Log(KLogInfo, "%s requested keystream for keys: %08x %08x", source.GetIpAddress().c_str(), key1, key2);

	bstring ret;
	ret.Assign("\x00\x81", 2);
	ret.Append((cstring)&key1, 4);
	ret.Append((cstring)&key2, 4);
	ret.Append(keystream, 80);
	ret.Append("\x01\x00", 2);
	return ret;
}

static bstring ChecksumRequest(const InetAddress & source, const bstring & packet)
{
	if(packet.GetSize() < 6)
		return bstring();

	uint32 seed = ((uint32 *)(packet.Data() + 2))[0];
	uint32 checksum = GenerateExeChecksum(seed);

	Logger::Instance().Log(KLogInfo, "%s requested executable checksum with seed: %08x", source.GetIpAddress().c_str(), seed);

	bstring ret;
	ret.Assign("\x00\x83", 2);
	ret.Append((cstring)&checksum, 4);
	return ret;
}

static bstring HalfstreamRequest(const InetAddress & source, const bstring & packet)
{
	if(packet.GetSize() < 6)
		return bstring();

	uint32 key = *(uint32 *)(packet.Data() + 2);
	uint8 keystream[80];
	GenerateHalfstream(keystream, key);

	Logger::Instance().Log(KLogInfo, "%s requested half keystream using key: %08x", source.GetIpAddress().c_str(), key);

	bstring ret;
	ret.Assign("\x00\x85", 2);
	ret.Append(keystream, 80);
	return ret;
}

static bstring HandlePacket(const InetAddress & source, const bstring & packet)
{
	if(packet.GetLength() < 2 || packet[0] != 0x00)
		return bstring();

	switch(packet[1])
	{
		case 0x80: return KeystreamRequest(source, packet);
		case 0x82: return ChecksumRequest(source, packet);
		case 0x84: return HalfstreamRequest(source, packet);
		default: return bstring();
	}
}


static bool IsBanned(const InetAddress & address)
{
	static const std::set <InetAddress> KBannedAddresses =
		{
			InetAddress("24.0.252.113")
		};

	return KBannedAddresses.find(address) != KBannedAddresses.end();
}

nativeint main(nativeint argc, const_cstring * argv)
{
	//
	// This was copied from Nucleus' main.cpp since wine doesn't play
	// nice with linking the main() function from another static library.
	// Note that we check for the '.exe.so' suffix rather than just '.exe.'
	//
	string procName = argv[0];
	string::size_type off = procName.rfind('/');
	if(off != string::npos)
		procName = procName.substr(off + 1);

	off = procName.rfind('\\');
	if(off != string::npos)
		procName = procName.substr(off + 1);

	string::size_type length = procName.length();
	if(length > 7 && procName.substr(length - 7) == ".exe.so")
		procName = procName.substr(0, length - 7);

	Process::SetName(procName.c_str());

	//
	// Begin app-specific code.
	//
	Logger & logger = Logger::Instance();
	try
	{
		Configuration config;
		config.LoadFromArguments(argv, argc);

		if(loader.Load("Continuum40.exe") == 0)
			throw system_error("Unable to map Continuum 0.40 executable into process address space.");

		InitializeContinuum();

		if(argc == 3)
		{
			if(std::string(argv[1]) == "-")
			{
				uint32 key2 = strtoul(argv[2], 0, 0);
				uint8 keystream[80];
				GenerateStep12(keystream, key2);

				bstring key(keystream, 80);
				printf("Key2: 0x%08x\n%s\n", key2, key.AsString().c_str());
			}
			else if(std::string(argv[2]) == "-")
			{
				uint32 key2 = strtoul(argv[1], 0, 0);
				uint8 keystream[80];
				GenerateHalfstream(keystream, key2);

				bstring key(keystream, 80);
				printf("Key2: 0x%08x\n%s\n", key2, key.AsString().c_str());
			}
			else
			{
				uint32 key1 = strtoul(argv[1], 0, 0);
				uint32 key2 = strtoul(argv[2], 0, 0);

				uint8 keystream[80];
				GenerateKeystream(keystream, key1, key2);

				bstring key(keystream, 80);
				printf("Key: 0x%08x, 0x%08x\n%s\n", key1, key2, key.AsString().c_str());
			}
			return 0;
		}
		else if(argc == 2)
		{
			uint32 key = strtoul(argv[1], 0, 0);

			uint32 csum = GenerateExeChecksum(key);
			printf("Checksum: 0x%08x -> 0x%08x\n", key, csum);
			return 0;
		}

		UdpClient server;
		server.SetNonBlocking();
		server.Bind(config.GetListenPort());

		logger.Log(KLogInfo, "Oracle server started on port %d.", config.GetListenPort());

		Selector selector;
		server.Register(selector, Selector::KRead);
		for(;;)
		{
			try
			{
				selector.Select();
				if(!selector.ReadSet().empty())
				{
					InetAddress addr;
					bstring packet = server.Read(addr, 1024);

					if(!IsBanned(addr))
					{
						packet = HandlePacket(addr, packet);

						if(!packet.IsEmpty())
							server.Write(addr, packet);
					}
					else
						logger.Log(KLogWarning, "Dropping packet from banned IP: %s", addr.AsString().c_str());
				}
			}
			catch(...)
			{
			}
		}
	}
	catch(const invalid_argument & err)
	{
		logger.Log(KLogError, "Invalid argument: %s\n", err.what());
	}
	catch(const system_error & err)
	{
		logger.Log(KLogError, "System error: %s\n", err.what());
	}

	return 0;
}

/*++

Copyright (c) Sharvil Nanavati.

Module:

    main.cpp

Authors:

    Sharvil Nanavati (sharvil) 2006-08-06

--*/

#include "base.h"
#include "NetLib.h"
#include "Nucleus.h"
#include "TimeLib.h"
#include "PeLoader.h"
#include "Configuration.h"

#include <iostream>
#include <windows.h>

PeLoader loader;

static void InitializeContinuum()
{
	//
	// Set up initial environment for Continuum executable. It needs a
	// private heap and a TLS area for its exception mechanism.
	//
	*(HANDLE *)loader.RVA(0x0BF20C) = HeapCreate(0, 0, 0);
	*(DWORD *)loader.RVA(0x0B5F80) = TlsAlloc();

	//
	// The Continuum packer overwrites the import table when loading the
	// executable. The game then (accidentally) uses some of the overwritten
	// import table values for the EXE checksum. The following overwrites the
	// relevant parts of the import table.
	//
	for(uint32 * ptr = (uint32 *)loader.RVA(0x0AFE50); ptr <= (uint32 *)loader.RVA(0x0AFE5C); ++ptr)
		*ptr = (uint32)loader.RVA(0x0B01E4);
}

static void GenerateKeystream(void * keystream, uint32 key1, uint32 key2)
{
	void ** ksptr = &keystream;
	void * base = loader.RVA(0);

	asm(
		"movl %0, %%ecx\n"
		"pushl %1\n"
		"movl $0x054740, %%eax\n"
		"addl %3, %%eax\n"
		"call *%%eax\n"

		"movl %0, %%ecx\n"
		"pushl %1\n"
		"movl $0x0555C0, %%eax\n"
		"addl %3, %%eax\n"
		"call *%%eax\n"

		"movl %0, %%ecx\n"
		"pushl %2\n"
		"movl $0x055870, %%eax\n"
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
		"movl $0x054740, %%eax\n"
		"addl %2, %%eax\n"
		"call *%%eax\n"

		:
		: "m" (ksptr), "m" (key), "m" (base)
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
		: "m" (seed), "r" (loader.RVA(0x04DF50))
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

	//printf("KEY %s %08x %08x\n", source.GetIpAddress().c_str(), key1, key2);

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

	//printf("CSUM %s %08x\n", source.GetIpAddress().c_str(), seed);

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

	//printf("HALF %s %08x\n", source.GetIpAddress().c_str(), key);

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

int32 main(int32 argc, const_cstring * argv)
{
	try
	{
		Configuration config;
		config.LoadFromArguments(argv, argc);

		if(loader.Load("Continuum38.exe") == 0)
			throw system_error("Unable to map Continuum 0.38 executable into process address space.");

		InitializeContinuum();

		UdpClient server;
		server.SetNonBlocking();
		server.Bind(config.GetListenPort());

		printf("Oracle server running on port %d.\n", config.GetListenPort());

		Selector selector;
		server.Register(selector, Selector::KRead);
		for(;;)
		{
			selector.Select();
			if(!selector.ReadSet().empty())
			{
				InetAddress addr;
				bstring packet = server.Read(addr, 1024);

				packet = HandlePacket(addr, packet);

				if(!packet.IsEmpty())
					server.Write(addr, packet);
			}
		}
	}
	catch(const invalid_argument & err)
	{
		printf("Invalid argument: %s\n", err.what());
	}
	catch(const system_error & err)
	{
		printf("System error: %s\n", err.what());
	}

	return 0;
}

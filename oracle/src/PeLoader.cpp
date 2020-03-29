/*++

Copyright (c) Sharvil Nanavati.

Module:

    PeLoader.cpp

Authors:

    Sharvil Nanavati (sharvil) 2006-08-06

--*/

#include "base.h"
#include "PeLoader.h"
#include "Nucleus.h"

#include <iostream>
#include <vector>
#include <sys/mman.h>

void * PeLoader::RVA(uint32 virtualOffset)
{
	return (void *)(iExecutableBase + virtualOffset);
}

void * PeLoader::Load(const std::string & filename)
{
	File fp(filename, "rb");

	MzHeader dosHeader;
	fp.Read(&dosHeader, sizeof(MzHeader));
	if(dosHeader.iMagic != 0x5A4D)
		throw invalid_argument("File does not contain DOS executable header.");

	fp.Seek(dosHeader.iPeOffset, File::KSeekAbsolute);

	PeHeader exeHeader;
	fp.Read(&exeHeader, sizeof(PeHeader));
	if(exeHeader.iPeMagic != 0x00004550)
		throw invalid_argument("File does not contain PE header.");

	std::vector <SectionHeader> headers;
	for(uint32 i = 0; i < exeHeader.iSectionCount; ++i)
	{
		SectionHeader header;
		fp.Read(&header, sizeof(SectionHeader));
		headers.push_back(header);
	}

	iExecutableBase = (uint32)mmap((void *)exeHeader.iImageBase, exeHeader.iSizeOfImage, PROT_EXEC | PROT_READ | PROT_WRITE, MAP_FIXED | MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
//	iExecutableBase = (uint32)VirtualAlloc((LPVOID)0/*exeHeader.iImageBase*/, exeHeader.iSizeOfImage, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
	if(iExecutableBase == 0)
		return 0;

	//
	// Load all sections into memory from disk.
	//
	for(std::vector <SectionHeader>::iterator i = headers.begin(); i != headers.end(); ++i)
	{
		fp.Seek(i->iDataOffset, File::KSeekAbsolute);
		fp.Read(RVA(i->iVirtualAddress), i->iDataSize);
	}

	//
	// Load all of the imports and fix up the import address table.
	//
	ImportDirectory * directory = (ImportDirectory *)RVA(exeHeader.iDataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].iVirtualAddress);
	while(directory->iRvaModuleName)
	{
		std::string libName((cstring)RVA(directory->iRvaModuleName));

		HMODULE module = LoadLibrary(libName.c_str());

		uint32 * originalThunk = (uint32 *)RVA(directory->iOriginalFirstThunk);
		uint32 * outputThunk = (uint32 *)RVA(directory->iRvaImportAddressTable);
		while(module && *originalThunk)
		{
			//
			// Only import functions by name and not ordinal.
			//
			if(!(*originalThunk & 0x80000000))
			{
				std::string funcName((cstring)RVA(*originalThunk + 2));
				*outputThunk = (uint32)GetProcAddress(module, funcName.c_str());
			}
			else
			{
				uint32 ordinal = *originalThunk & ~0x80000000;
				*outputThunk = (uint32)GetProcAddress(module, (const_cstring)ordinal);
			}

			++outputThunk;
			++originalThunk;
		}

		++directory;
	}

	//
	// Return the base address.
	//
	return (void *)RVA(0);
}

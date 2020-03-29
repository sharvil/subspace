/*++

Copyright (c) Sharvil Nanavati.

Module:

    PeLoader.h

Authors:

    Sharvil Nanavati (sharvil) 2006-08-06

--*/

#pragma once

#include "base.h"

#define IMAGE_DIRECTORY_ENTRY_EXPORT          0   // Export Directory
#define IMAGE_DIRECTORY_ENTRY_IMPORT          1   // Import Directory
#define IMAGE_DIRECTORY_ENTRY_RESOURCE        2   // Resource Directory
#define IMAGE_DIRECTORY_ENTRY_EXCEPTION       3   // Exception Directory
#define IMAGE_DIRECTORY_ENTRY_SECURITY        4   // Security Directory
#define IMAGE_DIRECTORY_ENTRY_BASERELOC       5   // Base Relocation Table
#define IMAGE_DIRECTORY_ENTRY_DEBUG           6   // Debug Directory
#define IMAGE_DIRECTORY_ENTRY_ARCHITECTURE    7   // Architecture Specific Data
#define IMAGE_DIRECTORY_ENTRY_GLOBALPTR       8   // RVA of GP
#define IMAGE_DIRECTORY_ENTRY_TLS             9   // TLS Directory
#define IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG    10   // Load Configuration Directory
#define IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT   11   // Bound Import Directory in headers
#define IMAGE_DIRECTORY_ENTRY_IAT            12   // Import Address Table
#define IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT   13   // Delay Load Import Descriptors
#define IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR 14   // COM Runtime descriptor

class PeLoader
{
	public:
		void * Load(const std::string & filename);
		void * RVA(uint32 virtualOffset);

	private:
		struct MzHeader
		{
			uint16 iMagic;
			uint16 iBytesOnLastPageOfFile;
			uint16 iPagesInFile;
			uint16 iRelocationCount;
			uint16 iSizeOfHeaderInParagraphs;
			uint16 iMinimumExtraParagraphsNeeded;
			uint16 iMaximumExtraParagraphsNeeded;
			uint16 iSs;
			uint16 iSp;
			uint16 iChecksum;
			uint16 iIp;
			uint16 iCs;
			uint16 iRelocationOffset;
			uint16 iOverlayNumber;
			uint16 iReserved0[4];
			uint16 iOemId;
			uint16 iOemInfo;
			uint16 iReserved1[10];
			uint32 iPeOffset;
		};

		struct PeHeader
		{
			uint32 iPeMagic;
			uint16 iMachine;
			uint16 iSectionCount;
			uint32 iTimestamp;
			uint32 iSymbolTableOffset;
			uint32 iSymbolCount;
			uint16 iNextHeaderSize;
			uint16 iCharacteristics;

			// --

			uint16 iTypeMagic;
			uint8 iMajorLinkerVersion;
			uint8 iMinorLinkerVersion;
			uint32 iSizeOfCode;
			uint32 iSizeOfInitializedData;
			uint32 iSizeOfUninitializedData;
			uint32 iEntryPointAddress;
			uint32 iBaseOfCode;
			uint32 iBaseOfData;
			uint32 iImageBase;
			uint32 iSectionAlignment;
			uint32 iFileAlignment;
			uint16 iMajorOsVersion;
			uint16 iMinorOsVersion;
			uint16 iMajorImageVersion;
			uint16 iMinorImageVersion;
			uint16 iMajorSubsystemVersion;
			uint16 iMinorSubsystemVersion;
			uint32 iWin32VersionValue;          // Unused
			uint32 iSizeOfImage;
			uint32 iSizeOfHeaders;
			uint32 iChecksum;
			uint16 iSubsystem;
			uint16 iDllCharacteristics;
			uint32 iSizeOfStackReserve;
			uint32 iSizeOfStackCommit;
			uint32 iSizeOfHeapReserve;
			uint32 iSizeOfHeapCommit;
			uint32 iLoaderFlags;               // Obsolete
			uint32 iNumberOfRvaAndSizes;
			struct
			{
				uint32 iVirtualAddress;
				uint32 iSize;
			} iDataDirectory[16];
		};

		struct ImportDirectory
		{
			union
			{
				uint32 iCharacteristics;
				uint32 iOriginalFirstThunk;
			};
			uint32 iTimeDateStamp;
			uint32 iForwarderChain;
			uint32 iRvaModuleName;
			uint32 iRvaImportAddressTable;
		};

		struct SectionHeader
		{
			int8 iName[8];
			uint32 iVirtualSize;
			uint32 iVirtualAddress;
			uint32 iDataSize;
			uint32 iDataOffset;
			uint32 iRelocationOffset;
			uint32 iLineNumberOffset;
			uint16 iRelocationCount;
			uint16 iLineNumberCount;
			uint32 iCharacteristics;
		};

	private:
		uint32 iExecutableBase;
};

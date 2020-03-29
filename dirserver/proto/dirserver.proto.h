#pragma once

#include "base.h"

struct ZoneUpdatePacket
{
	template <typename T> using stream_reader_t = StreamDataReader <T, LittleEndian, Ascii>;
	typedef DataReader <LittleEndian, Ascii> reader_t;
	typedef DataWriter <LittleEndian, Ascii> writer_t;

	ZoneUpdatePacket()
	{
		iIp = 0x00000000;
		iHopCount = 0x00000000;
		iServerVersion = 0x00000086;
		iTerminator = 0x00000000;
	}

	bstring Serialize() const
	{
		writer_t serializer;
		serializer.AppendUint32(iIp);
		serializer.AppendUint16(iPort);
		serializer.AppendUint16(iPlayerCount);
		serializer.AppendBool(iScoreKeeping);
		serializer.AppendUint8(iHopCount);
		serializer.AppendUint32(iServerVersion);
		string iName_ = iName;
		iName_.resize(0x00000020);
		serializer.AppendString(iName_);
		string iPassword_ = iPassword;
		iPassword_.resize(0x00000030);
		serializer.AppendString(iPassword_);
		serializer.AppendString(iDescription);
		serializer.AppendUint8(iTerminator);
		return serializer.Asbstring();
	}

	void Deserialize(const bstring & data)
	{
		reader_t serializer(data);
		Deserialize(serializer);
	}

	template <typename T>
	void Deserialize(stream_reader_t <T> & serializer)
	{
		iIp = serializer.ReadUint32();
		iPort = serializer.ReadUint16();
		iPlayerCount = serializer.ReadUint16();
		iScoreKeeping = serializer.ReadBool();
		iHopCount = serializer.ReadUint8();
		iServerVersion = serializer.ReadUint32();
		iName = serializer.ReadString(0x00000020);
		iPassword = serializer.ReadString(0x00000030);
		iDescription = serializer.ReadString();
		iTerminator = serializer.ReadUint8();
	}

	void Deserialize(reader_t & serializer)
	{
		iIp = serializer.ReadUint32();
		iPort = serializer.ReadUint16();
		iPlayerCount = serializer.ReadUint16();
		iScoreKeeping = serializer.ReadBool();
		iHopCount = serializer.ReadUint8();
		iServerVersion = serializer.ReadUint32();
		iName = serializer.ReadString(0x00000020);
		iPassword = serializer.ReadString(0x00000030);
		iDescription = serializer.ReadString();
		iTerminator = serializer.ReadUint8();
	}

	uint32 iIp;
	uint16 iPort;
	uint16 iPlayerCount;
	bool iScoreKeeping;
	uint8 iHopCount;
	uint32 iServerVersion;
	string iName;
	string iPassword;
	string iDescription;
	uint8 iTerminator;
};

struct ZonePacket
{
	template <typename T> using stream_reader_t = StreamDataReader <T, LittleEndian, Ascii>;
	typedef DataReader <LittleEndian, Ascii> reader_t;
	typedef DataWriter <LittleEndian, Ascii> writer_t;

	ZonePacket()
	{
		iIp = 0x00000000;
		iHopCount = 0x00000000;
		iServerVersion = 0x00000086;
		iTerminator = 0x00000000;
	}

	bstring Serialize() const
	{
		writer_t serializer;
		serializer.AppendUint32(iIp);
		serializer.AppendUint16(iPort);
		serializer.AppendUint16(iPlayerCount);
		serializer.AppendBool(iScoreKeeping);
		serializer.AppendUint8(iHopCount);
		serializer.AppendUint32(iServerVersion);
		string iName_ = iName;
		iName_.resize(0x00000040);
		serializer.AppendString(iName_);
		serializer.AppendString(iDescription);
		serializer.AppendUint8(iTerminator);
		return serializer.Asbstring();
	}

	void Deserialize(const bstring & data)
	{
		reader_t serializer(data);
		Deserialize(serializer);
	}

	template <typename T>
	void Deserialize(stream_reader_t <T> & serializer)
	{
		iIp = serializer.ReadUint32();
		iPort = serializer.ReadUint16();
		iPlayerCount = serializer.ReadUint16();
		iScoreKeeping = serializer.ReadBool();
		iHopCount = serializer.ReadUint8();
		iServerVersion = serializer.ReadUint32();
		iName = serializer.ReadString(0x00000040);
		iDescription = serializer.ReadString();
		iTerminator = serializer.ReadUint8();
	}

	void Deserialize(reader_t & serializer)
	{
		iIp = serializer.ReadUint32();
		iPort = serializer.ReadUint16();
		iPlayerCount = serializer.ReadUint16();
		iScoreKeeping = serializer.ReadBool();
		iHopCount = serializer.ReadUint8();
		iServerVersion = serializer.ReadUint32();
		iName = serializer.ReadString(0x00000040);
		iDescription = serializer.ReadString();
		iTerminator = serializer.ReadUint8();
	}

	uint32 iIp;
	uint16 iPort;
	uint16 iPlayerCount;
	bool iScoreKeeping;
	uint8 iHopCount;
	uint32 iServerVersion;
	string iName;
	string iDescription;
	uint8 iTerminator;
};

struct DirectoryListRequest
{
	template <typename T> using stream_reader_t = StreamDataReader <T, LittleEndian, Ascii>;
	typedef DataReader <LittleEndian, Ascii> reader_t;
	typedef DataWriter <LittleEndian, Ascii> writer_t;

	DirectoryListRequest()
	{
		iType = 0x00000001;
		iMinPlayerCount = 0x00000000;
	}

	bstring Serialize() const
	{
		writer_t serializer;
		serializer.AppendUint8(iType);
		serializer.AppendUint32(iMinPlayerCount);
		return serializer.Asbstring();
	}

	void Deserialize(const bstring & data)
	{
		reader_t serializer(data);
		Deserialize(serializer);
	}

	template <typename T>
	void Deserialize(stream_reader_t <T> & serializer)
	{
		iType = serializer.ReadUint8();
		iMinPlayerCount = serializer.ReadUint32();
	}

	void Deserialize(reader_t & serializer)
	{
		iType = serializer.ReadUint8();
		iMinPlayerCount = serializer.ReadUint32();
	}

	uint8 iType;
	uint32 iMinPlayerCount;
};

#include "common.h"
#include "SubSpaceProtocol.h"
#include "SSNetLib.h"
#include "TimeLib.h"
#include "CryptoLib.h"
#include "zlib.h"
#include "LogLib.h"

#include <sstream>

const std::string SubSpaceProtocol::KFilePath = "data/files/";
const std::string SubSpaceProtocol::KMapPath = "data/maps/";

SubSpaceProtocol::SubSpaceProtocol()
	: iUid(KInvalidPlid),
	  iNewUser(false),
	  iMapData(0),
	  iSettings(0),
	  iLastPositionUpdate(0),
	  iLastSecurityKey(0),
	  iIsConnected(false),
	  iIsLoggedIn(false),
	  iInArena(false)
{
}

SubSpaceProtocol::SubSpaceProtocol(const std::string & username, const std::string & password)
	: iUid(KInvalidPlid),
	  iUsername(username),
	  iPassword(password),
	  iLastPositionUpdate(0),
	  iLastSecurityKey(0),
	  iIsConnected(false),
	  iIsLoggedIn(false),
	  iInArena(false)
{
}

SubSpaceProtocol::~SubSpaceProtocol()
{
	delete [] iMapData;
	delete [] iSettings;
}

bool SubSpaceProtocol::IsConnected() const
{
	return iIsConnected;
}

bool SubSpaceProtocol::IsLoggedIn() const
{
	return iIsLoggedIn;
}

bool SubSpaceProtocol::IsInArena() const
{
	return iInArena;
}

void SubSpaceProtocol::GetArenaList()
{
	SendChatMessage(KInvalidPlid, KChatPublic, "?arena");
}

void SubSpaceProtocol::ChangeShip(uint8 ship)
{
	bstring packet(2);
	packet[0] = 0x18;
	packet[1] = ship;
	iWriteQueue.push_back(std::make_pair(packet, true));
}

void SubSpaceProtocol::EnterArena(const std::string & arenaName)
{
	uint16 arenaId = 0xFFFF;

	if(!arenaName.empty())
	{
		cstring endptr = 0;
		uint32 id = strtoul(arenaName.c_str(), &endptr, 0);
		if(*endptr || id > 0x7FFF)
			arenaId = 0xFFFD;
		else
			arenaId = id;
	}

	std::string arenaStr;
	if(arenaId == 0xFFFD)
		arenaStr = arenaName;
	arenaStr.resize(16);

	writer_t writer;
	writer.AppendUint8(0x01);
	writer.AppendUint8(0x08);
	writer.AppendUint16(0x01);
	writer.AppendUint16(1024);
	writer.AppendUint16(768);
	writer.AppendUint16(arenaId);
	writer.AppendString(arenaStr);

	iWriteQueue.push_back(std::make_pair(writer.Asbstring(), true));

	iInArena = false;
}

void SubSpaceProtocol::SendChatMessage(plid_t to, const EChatType & type, const std::string & message)
{
	gsize read, written;
	gchar * tmp = g_convert(message.c_str(), message.length(), "ISO-8859-1", "UTF-8", &read, &written, 0);

	if(tmp == 0)
		return;

	std::string utfMsg = std::string(tmp, written);
	g_free(tmp);

	DataWriter <LittleEndian, AsciiZ> writer;
	writer.AppendUint8(0x06);
	writer.AppendUint8(type);
	writer.AppendUint8(0x00);
	writer.AppendUint16(to);
	writer.AppendString(utfMsg);

	iWriteQueue.push_back(std::make_pair(writer.Asbstring(), true));
}

void SubSpaceProtocol::SendRegistrationForm(std::string realName, std::string emailAddress)
{
	realName.resize(32);
	emailAddress.resize(64);

	DataWriter <LittleEndian, Ascii> writer;

	writer.AppendUint8(0x17);
	writer.AppendString(realName);
	writer.AppendString(emailAddress);
	writer.AppendString(bstring(766-1-32-64));

	iWriteQueue.push_back(std::make_pair(writer.Asbstring(), true));
}

void SubSpaceProtocol::Poll(SSConnection & connection)
{
	if(!IsLoggedIn())
		return;

	for(WriteQueue::iterator i = iWriteQueue.begin(); i != iWriteQueue.end(); ++i)
		connection.Send(i->first, i->second);
	iWriteQueue.clear();

	if(!IsInArena())
		return;

	uint32 now = Time::GetMilliCount() / 10;
	if(now - iLastPositionUpdate >= 40)
	{
		uint32 serverTime = connection.ToRemoteTimestamp(now);

		writer_t writer;
		writer.AppendUint8(0x03);               // Position packet
		writer.AppendUint8(0x00);               // Direction
		writer.AppendUint32(serverTime);        // Timestamp (remote)
		writer.AppendUint16(0);                 // X velocity
		writer.AppendUint16(8192);              // Y location
		writer.AppendUint8(0);                  // Packet checksum
		writer.AppendUint8(0);                  // Togglables
		writer.AppendUint16(8192);              // X location
		writer.AppendUint16(0);                 // Y velocity
		writer.AppendUint16(0);                 // Bounty
		writer.AppendUint16(0);                 // Energy
		writer.AppendUint16(0);                 // Weapon info

		bstring packet = writer.Asbstring();
		uint8 checksum = 0;
		for(bstring::size_type j = 0; j < packet.GetLength(); ++j)
			checksum ^= packet[j];

		packet[10] = checksum;

		connection.Send(packet);

		iLastPositionUpdate = now;
	}

	OnPoll();
}

void SubSpaceProtocol::OnConnect(SSConnection & connection)
{
	iWriteQueue.clear();

	std::string name = iUsername;
	std::string password = iPassword;
	bstring security("\x5F\x26\x5A\xC6\x29\xB9\xEB\x4F\x43\x56\x6A\x65\xFE\x8A\x55\xA8\x29\x85\x03\x4C\x0F\x25\xEF\x95\xF1\xC8\x8B\x43\x2A\x8D\xBF\x8C\xB1\x56\x5A\x9D\x62\x30\x2D\xE1\x2E\xE6\x13\x0A\x1E\xEE\x83\x61\x23\xC7\x80\x96\xB5\x07\xE3\xCC\x64\x33\x44\x29\x2F\x2E\xFD\x9E", 0x40);

	name.resize(32);
	password.resize(32);

	uint32 machineId = Time::GetMilliCount() & 0x7FFFFFFF;

	writer_t writer;

	writer.AppendUint8(0x24);
	writer.AppendUint8(iNewUser);      // Register new user
	writer.AppendString(name);         // User name
	writer.AppendString(password);     // Password
	writer.AppendUint32(machineId);    // Machine ID
	writer.AppendUint8(0x04);          // Connection type
	writer.AppendUint32(0x12C);        // Time zone bias
	writer.AppendUint16(40);           // Client version
	writer.AppendUint32(444);          // anims.dll version
	writer.AppendUint32(555);          // anims.dll version
	writer.AppendUint32(0);            // unknown!
	writer.AppendUint32(connection.GetRemoteAddress().AsUint32HostOrder());           // IP address of server
	writer.AppendUint32(0);            // unknown!
	writer.AppendUint32(0);            // unknown
	writer.AppendString(security);

	connection.Send(writer.Asbstring(), true);

	iIsConnected = true;
	iIsLoggedIn = false;
	iInArena = false;
}

void SubSpaceProtocol::OnReceive(SSConnection & connection, const bstring & packet)
{
	if(packet.GetLength() < 1)
		return;

	switch(packet[0])
	{
		case 0x00: ProcessOracleResponse(connection, packet); break;
		case 0x01: ProcessUidAssignment(connection, packet); break;
		case 0x02: ProcessArenaLoginComplete(connection, packet); break;
		case 0x03: ProcessPlayerLogin(connection, packet); break;
		case 0x04: ProcessPlayerLogout(connection, packet); break;
		case 0x05: /* Weapon packet */ break;
		case 0x06: ProcessPlayerDied(connection, packet); break;
		case 0x07: ProcessChatMessage(connection, packet); break;
		case 0x08: /* Player Prize */ break;
		case 0x0A: ProcessLoginResponse(connection, packet); break;
		case 0x0F: ProcessArenaSettings(connection, packet); break;
		case 0x10: ProcessFileTransfer(connection, packet); break;
		case 0x12: /* Flag Position */ break;
		case 0x18: ProcessSynchronizeRequest(connection, packet); break;
		case 0x19: ProcessFileRequest(connection, packet); break;
		case 0x1D: ProcessShipTeamChange(connection, packet); break;
		case 0x21: /* Brick drop */ break;
		case 0x27: /* Keep alive */ break;
		case 0x28: /* Position */ break;
		case 0x29: ProcessMapMetadata(connection, packet); break;
		case 0x2A: ProcessMapData(connection, packet); break;
		case 0x2F: ProcessArenaListing(connection, packet); break;
		case 0x31: ProcessLoginComplete(connection, packet); break;
		case 0x33: ProcessLoginMessage(connection, packet); break;
		case 0x34: /* Continuum version */ break;
		case 0x3B: /* *sendto: 3B <ip (4)> <port (2)> <arena type (2)> <arena name (16)> <something (4)> */ break;
		default: /* Logger::Instance().Log(KLogDebug, "%s\n", packet.AsString().c_str()); */ break;
	}
}

void SubSpaceProtocol::OnDisconnect()
{
	iInArena = false;
	iIsLoggedIn = false;
	iIsConnected = false;

	OnLogout();
}

void SubSpaceProtocol::LoadMap(const bstring & data)
{
	reader_t reader(data);
	if(data[0] == 'B' && data[1] == 'M')
	{
		reader.ReadUint16();
		reader.Readbstring(reader.ReadUint32() - 2);
	}

	if(iMapData == 0)
		iMapData = new uint8[1024 * 1024];

	memset(iMapData, 0, 1024 * 1024);

	while(!reader.IsEndOfData())
	{
		uint32 record = reader.ReadUint32();

		uint32 x = record & 0x3FF;
		uint32 y = (record >> 12) & 0x3FF;
		uint8 tile = record >> 24;

		iMapData[x + y * 1024] = tile;
	}
}

uint32 SubSpaceProtocol::GenerateMapChecksum(uint32 key) const
{
	if(iMapData == 0)
		return 0;

	// Negative keys are ignored by the client
	if(key & 0x80000000)
		return key;

	uint32 ret = key;
	for(uint32 y = key % 32; y < 1024; y += 32)
		for(uint32 x = key % 31; x < 1024; x += 31)
		{
			uint8 tile = iMapData[x + y * 1024];
			if((tile >= 1 && tile <= 160) || tile == 171)
				ret += key ^ tile;
		}

	return ret;
}

uint32 SubSpaceProtocol::GenerateSettingsChecksum(uint32 key) const
{
	if(iSettings == 0)
		return 0;

	uint32 csum = 0;
	for(uint32 i = 0; i < 357; ++i)
		csum += (key ^ iSettings[i]);
	return csum;
}

void SubSpaceProtocol::ProcessOracleResponse(SSConnection & connection, const bstring & packet)
{
	if(packet.GetLength() != 6)
		throw malformed_error("oracle checksum", "[SubSpaceProtocol::ProcessOracleResponse]: invalid Oracle checksum response.");

	uint32 settingsCsum = GenerateSettingsChecksum(iLastSecurityKey);
	uint32 mapCsum = GenerateMapChecksum(iLastSecurityKey);

	reader_t reader(packet);
	reader.ReadUint16();

	writer_t writer;
	writer.AppendUint8(0x1A);
	writer.AppendUint32(0x00);                    // Weapon count
	writer.AppendUint32(settingsCsum);            // Parameter checksum (settings)
	writer.AppendUint32(reader.ReadUint32());     // EXE checksum
	writer.AppendUint32(mapCsum);                 // Map checksum
	writer.AppendUint32(0x00);                    // S2C Slow Total
	writer.AppendUint32(0x00);                    // S2C Fast Total
	writer.AppendUint16(0x00);                    // S2C Slow Current
	writer.AppendUint16(0x00);                    // S2C Fast Current
	writer.AppendUint16(0x00);                    // S2C Rel Out
	writer.AppendUint16(0x00);                    // Ping
	writer.AppendUint16(0x00);                    // Average ping
	writer.AppendUint16(0x00);                    // Low ping
	writer.AppendUint16(0x00);                    // High ping
	writer.AppendUint8(0x00);                     // Slow frame

	connection.Send(writer.Asbstring(), true);
}

void SubSpaceProtocol::ProcessUidAssignment(SSConnection & connection, const bstring & packet)
{
	reader_t reader(packet);
	reader.ReadUint8();

	iUid = reader.ReadUint16();
}

void SubSpaceProtocol::ProcessArenaLoginComplete(SSConnection & connection, const bstring & packet)
{
	iInArena = true;
	OnArenaLogin();
}

void SubSpaceProtocol::ProcessPlayerLogin(SSConnection & connection, const bstring & packet)
{
	DataReader <LittleEndian, Ascii> reader(packet);

	while(!reader.IsEndOfData())
	{
		Player p;

		reader.ReadUint8(); // type

		p.iShip = static_cast <EShipType> (reader.ReadUint8());

		reader.ReadUint8(); // accept audio

		p.iName = reader.ReadString(20);
		p.iSquad = reader.ReadString(20);
		p.iFlagPoints = reader.ReadUint32();
		p.iKillPoints = reader.ReadUint32();
		p.iId = reader.ReadUint16();
		p.iFrequency = reader.ReadUint16();
		p.iWins = reader.ReadUint16();
		p.iLosses = reader.ReadUint16();
		reader.ReadUint16(); // turreting
		p.iFlagsCarried = reader.ReadUint16();
		reader.ReadUint8(); // bitfield

		OnPlayerEntering(p);
	}
}

void SubSpaceProtocol::ProcessPlayerLogout(SSConnection & connection, const bstring & packet)
{
	reader_t reader(packet);
	reader.ReadUint8();
	OnPlayerLeaving(reader.ReadUint16());
}

void SubSpaceProtocol::ProcessPlayerDied(SSConnection & connection, const bstring & packet)
{
	reader_t reader(packet);
	reader.ReadUint8();

	uint8 prize = reader.ReadUint8();
	plid_t killer = reader.ReadUint16();
	plid_t killed = reader.ReadUint16();
	uint16 bounty = reader.ReadUint16();
	uint16 flags = reader.ReadUint16();

	UNUSED_VARIABLE(prize);
	UNUSED_VARIABLE(bounty);
	UNUSED_VARIABLE(flags);

	OnPlayerDied(killed, killer);
}

void SubSpaceProtocol::ProcessChatMessage(SSConnection & connection, const bstring & packet)
{
	reader_t reader(packet);
	reader.ReadUint8();
	EChatType type = static_cast <EChatType> (reader.ReadUint8());
	uint8 audio = reader.ReadUint8();
	plid_t uid = reader.ReadUint16();
	std::string message = reader.ReadString();

	UNUSED_VARIABLE(audio);

	if(type > KChatChannel)
		type = KChatArena;

	gsize read, written;
	gchar * utfMsg = g_convert(message.c_str(), message.length(), "UTF-8", "ISO-8859-1", &read, &written, NULL);

	if(utfMsg != 0)
	{
		message = std::string(utfMsg, written);
		g_free(utfMsg);

		OnChatMessage(uid, type, message);
	}
}

void SubSpaceProtocol::ProcessLoginResponse(SSConnection & connection, const bstring & packet)
{
	ELoginResponse response = static_cast <ELoginResponse> (packet[1]);
	bool requestRegistration = (packet[19] != 0);

	//
	// Ugly hack to make things work right between Isometry and Continuum's
	// registration form.
	//
	if(response == KLoginAskDemographics && requestRegistration)
		response = KLoginOk;

	iIsLoggedIn = (response == KLoginOk || response == KLoginSpectateOnly || response == KLoginBillerDown);
	OnLoginResponse(response, requestRegistration);

	if(!IsLoggedIn())
		connection.Disconnect();
}

void SubSpaceProtocol::ProcessArenaSettings(SSConnection & connection, const bstring & packet)
{
	delete [] iSettings, iSettings = new uint32 [357];

	reader_t reader(packet);
	for(uint32 i = 0; i < 357; ++i)
		iSettings[i] = reader.ReadUint32();
}

void SubSpaceProtocol::ProcessFileTransfer(SSConnection & connection, const bstring & packet)
{
	if(packet.GetLength() < 18)
		return;

	DataReader <LittleEndian, Ascii> reader(packet);
	reader.ReadUint8();
	std::string filename = reader.ReadString(16);
	bstring data;

	if(filename.empty())
	{
		filename = "news.txt";
		data = Uncompress(packet, 17);
	}
	else
		data = packet.SubString(17);

	filename = KFilePath + filename;

	File fp(filename, "wb");
	fp.Write(data);
	fp.Close();
}

void SubSpaceProtocol::ProcessShipTeamChange(SSConnection & connection, const bstring & packet)
{
	reader_t reader(packet);

	reader.ReadUint8();
	uint8 ship = reader.ReadUint8();
	plid_t player = reader.ReadUint16();
	uint16 team = reader.ReadUint16();

	OnShipTeamChange(player, ship, team);
}

void SubSpaceProtocol::ProcessSynchronizeRequest(SSConnection & connection, const bstring & packet)
{
	reader_t reader(packet);

	reader.ReadUint8();
	uint32 greenSeed = reader.ReadUint32();
	uint32 doorSeed = reader.ReadUint32();
	uint32 timestamp = reader.ReadUint32();
	iLastSecurityKey = reader.ReadUint32();

	UNUSED_VARIABLE(greenSeed);
	UNUSED_VARIABLE(doorSeed);
	UNUSED_VARIABLE(timestamp);

	if(iLastSecurityKey != 0)
		connection.RequestExecutableChecksum(iLastSecurityKey);
}

void SubSpaceProtocol::ProcessFileRequest(SSConnection & connection, const bstring & packet)
{
	DataReader <LittleEndian, Ascii> reader(packet);

	reader.ReadUint8();
	std::string localFilename = reader.ReadString(256);
	std::string remoteFilename = reader.ReadString(16);

	printf("Sending '%s' as '%s'\n", localFilename.c_str(), remoteFilename.c_str());

	remoteFilename.resize(16);

	try
	{
		File fp(localFilename, "rb");

		DataWriter <LittleEndian, Ascii> writer;
		writer.AppendUint8(0x16);
		writer.AppendString(remoteFilename);
		writer.AppendString(fp.Read(File::Length(localFilename)));

		connection.SendFile(writer.Asbstring());
	}
	catch(const std::runtime_error & error)
	{
		printf("Error sending file: %s\n", error.what());
	}
}

void SubSpaceProtocol::ProcessMapMetadata(SSConnection & connection, const bstring & packet)
{
	DataReader <LittleEndian, Ascii> reader(packet);
	reader.ReadUint8();

	std::string filename = KMapPath + reader.ReadString(16);
	uint32 checksum = reader.ReadUint32();

	if(!File::Exists(filename))
	{
		connection.Send(bstring("\x0C", 1));
		return;
	}

	File fp(filename, "rb");
	bstring data = fp.Read(File::Length(filename));
	fp.Close();

	Crc32 crc;
	if(crc.Checksum(data) != checksum)
	{
		connection.Send(bstring("\x0C", 1));
		return;
	}

	LoadMap(data);
}

void SubSpaceProtocol::ProcessMapData(SSConnection & connection, const bstring & packet)
{
	DataReader <LittleEndian, Ascii> reader(packet);
	reader.ReadUint8();
	std::string filename = KMapPath + reader.ReadString(16);
	bstring ucMap = Uncompress(packet, 17);

	File fp(filename, "wb");
	fp.Write(ucMap);
	fp.Close();

	LoadMap(ucMap);
}

void SubSpaceProtocol::ProcessArenaListing(SSConnection & connection, const bstring & packet)
{
	reader_t reader(packet);
	reader.ReadUint8();

	ArenaList arenas;
	while(!reader.IsEndOfData())
	{
		ArenaEntry entry;

		entry.iName = reader.ReadString();
		entry.iPopulation = reader.ReadInt16();
		entry.iInArena = entry.iPopulation < 0;

		bool isPub = true;
		for(std::string::size_type i = 0; isPub && i < entry.iName.length(); ++i)
			if(entry.iName[i] < '0' || entry.iName[i] > '9')
				isPub = false;

		if(isPub)
			entry.iDisplayName = "(Public " + entry.iName + ")";
		else
			entry.iDisplayName = entry.iName;

		entry.iPopulation = std::abs(entry.iPopulation);

		arenas.push_back(entry);
	}

	OnArenaList(arenas);
}

void SubSpaceProtocol::ProcessLoginComplete(SSConnection & connection, const bstring & packet)
{
}

void SubSpaceProtocol::ProcessLoginMessage(SSConnection & connection, const bstring & packet)
{
	reader_t reader(packet, 1);
	OnLoginMessage(reader.ReadString());
}

bstring SubSpaceProtocol::Uncompress(const bstring & data, bstring::size_type offset) const
{
	if(offset >= data.GetLength())
		throw invalid_argument("[SubSpaceProtocol::Uncompress]: offset out-of-bounds.");

	return Uncompress(data.Data() + offset, data.GetLength() - offset);
}

bstring SubSpaceProtocol::Uncompress(const uint8 * data, bstring::size_type length) const
{
	bstring ret;
	ret.Resize(length * 2);

	uLongf destLen = ret.GetLength();
	while(uncompress(&ret[0], &destLen, data, length) == Z_BUF_ERROR)
	{
		destLen = ret.GetLength() * 2;
		ret.Resize(destLen);
	}
	ret.Resize(destLen);

	return ret;
}

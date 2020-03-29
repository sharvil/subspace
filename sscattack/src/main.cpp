#include "base.h"
#include "SSNetLib.h"
#include "NetLib.h"
#include "TimeLib.h"

bstring MakePacket(uint16 port)
{
	std::string ServerName = "<redacted>";
	std::string Password = "<redacted>";
	uint32 ServerId = 14700;
	uint32 GroupId = 1;
	uint32 ScoreId = 14700;

	ServerName.resize(126);
	Password.resize(32);

	DataWriter <LittleEndian, Ascii> writer;
	writer.AppendUint8(0x02);
	writer.AppendUint32(ServerId);
	writer.AppendUint32(GroupId);
	writer.AppendUint32(ScoreId);
	writer.AppendString(ServerName);
	writer.AppendUint16(port);
	writer.AppendString(Password);

	return writer.Asbstring();
}

int main(int argc, const_cstring * argv)
{
/*
;SSC INFO
;IP = 207.44.214.98
;Port = 9010
;ServerName = <redacted>
;Password = <redacted>
;ServerId = 14700
;GroupId = 1
;ScoreId = 14700
*/
	if(argc != 5)
	{
		printf("Usage: %s <start> <end> <skip> <delay>\n", argv[0]);
		return 1;
	}

	uint32 start = strtol(argv[1], 0, 0);
	uint32 end = strtol(argv[2], 0, 0);
	uint32 skip = strtol(argv[3], 0, 0);
	uint32 delay = strtol(argv[4], 0, 0);

	if(!start || !end || !skip || end < start || start < 0 || end < 0 || skip < 0 || start > 65535 || end > 65535 || skip > 65535 || delay < 250 || delay > 100000)
	{
		printf("Usage: %s <start> <end> <skip> <delay>\n", argv[0]);
		return 1;
	}

	InetAddress SSC("207.44.214.98", 9010);
	Selector selector;

	for(uint32 port = start; port < end; port += skip)
	{
		printf("Attempting to grab ID for port %d\n", port);

		SSConnectionPool pool(selector);

		SSConnection * conn = pool.Connect(SSC);
		conn->Send(MakePacket(port), true);

		uint32 startTime = Time::GetMilliCount();
		for(bool next = false; !next; )
		{
			selector.Select(1000);

			while(!next && conn->HasMorePackets())
			{
				bstring packet = conn->GetNextPacket();
				if(packet[0] == 0x33)
				{
					File fp;
					fp.Open(Format("data/sscid_%d.dat", port), "wb");
					fp.Write(packet.SubString(1));
					next = true;
					printf("+ Grabbed sscid_%d!\n", port);
					conn->Disconnect();
				}
			}

			if(Time::GetMilliCount() - startTime >= 10000)
			{
				conn->Disconnect();
				printf("*** Failed sscid_%d!!!!\n", port);
				next = true;
			}
		}

		delete conn;
		Time::Sleep(delay);
	}

	return 0;
}

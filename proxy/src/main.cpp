#include "base.h"
#include "NetLib.h"
#include "TimeLib.h"
#include "CryptoLib.h"

#if !defined(PLATFORM_WIN32)
#	include <signal.h>
#endif

#include <cstring>
#include <iostream>

bool gQuitSignal = false;
Selector selector;

#if defined(PLATFORM_WIN32)
	BOOL WINAPI quitHandler(DWORD type)
	{
		gQuitSignal = true;
		selector.Alert();
		return TRUE;
	}

	void setSignalHandler()
	{
		SetConsoleCtrlHandler(quitHandler, true);
	}
#else
	void quitHandler(int32)
	{
		gQuitSignal = true;
	}

	void setSignalHandler()
	{
		struct sigaction quitSignal;
		memset(&quitSignal, 0, sizeof(quitSignal));
		quitSignal.sa_handler = quitHandler;
		if(sigaction(SIGINT, &quitSignal, 0) < 0)
			throw std::runtime_error("Unable to set signal handler.");
	}
#endif

int32 main(int32 argc, const_cstring * argv)
{
	try
	{
		setSignalHandler();

//		std::string ip = "66.36.241.110";
//		uint16 port = 5400;
		std::string ip = "68.74.145.185";
		uint16 port = 8000;
		uint16 listenPort = 6001;

		if(argc > 2)
		{
			ip = argv[1];
			sscanf(argv[2], "%hd", &port);
		}
		if(argc > 3)
			sscanf(argv[3], "%hd", &listenPort);

		UdpClient server;
		server.Bind(listenPort);
		server.Register(selector, Selector::KRead);

		std::cout << "Listening on: " << listenPort << " and proxying to: " << ip << ":" << port << std::endl;

		ICryptosystem * encryptor = 0;
		bool encrypting = false;
		bool continuum = false;
		InetAddress defaultServerAddress(ip, port);
		InetAddress oracleAddress("sharvil.nanavati.net", 6000);
		InetAddress clientAddress;
		InetAddress serverAddress;
		while(!gQuitSignal)
		{
			selector.Select();

			if(selector.ReadSet().empty())
				continue;

			bool filter = false;
			InetAddress source;
			bstring packet = server.Read(source, 65536);

			//
			// Check for connection packets.
			//
			if(packet.length() == 8 && packet[0] == 0x00 && packet[1] == 0x01 && packet[7] == 0x00)
			{
				delete encryptor, encryptor = 0;
				continuum = false;
				encrypting = false;
				clientAddress = source;
				serverAddress = defaultServerAddress;
			}

			//
			// Key negotiation packets.
			//
			if(packet.length() == 12 && packet[0] == 0x00 && packet[1] == 0x10)
			{
				packet[1] = 0x80;
				server.Write(oracleAddress, packet);
				filter = true;
				continuum = true;
			}

			//
			// Oracle response
			//
			if(packet.length() == 92 && packet[0] == 0x00 && packet[1] == 0x81)
			{
				DataReader <LittleEndian> reader(packet);

				bstring forwardPacket = reader.Readbstring(10);
				encryptor = new ContinuumCrypt(reader.Readbstring(80));
				forwardPacket.Append(reader.Readbstring(2));

				forwardPacket[1] = 0x10;
				server.Write(clientAddress, forwardPacket);
				std::cout << forwardPacket.AsString() << std::endl;
				filter = true;
			}

			//
			// Transport packets along as necessary
			//
			if(!filter)
			{
				if(source == clientAddress)
				{
					std::cout << "-C2S-" << std::endl;
					server.Write(serverAddress, packet);
				}
				else if(source == serverAddress)
				{
					std::cout << "-S2C-" << std::endl;
					server.Write(clientAddress, packet);
				}
				else if(source == oracleAddress)
				{
					std::cout << "-O2P-" << std::endl;
				}
				else
					std::cout << "Dropping packet from unknown origin." << std::endl;

				if(encrypting)
				{
					if(packet[0] == 0xFF)
						packet = packet.SubString(1);
					encryptor->Decrypt(packet);
					if(continuum)
						packet = packet.SubString(1);
				}

				std::cout << packet.AsString() << std::endl;
			}

			//
			// Set up the encryptor once we've got a valid 00 11 response from
			// the client.
			//
			if(packet.length() == 8 && packet[0] == 0x00 && packet[1] == 0x11 && encryptor != 0)
			{
				encrypting = true;
				std::cout << "         -- Continuum Encryption -- " << std::endl;
			}
			else if(packet.length() >= 6 && packet[0] == 0x00 && packet[1] == 0x02 && encryptor == 0 && !continuum)
			{
				DataReader <LittleEndian> reader(packet);
				reader.ReadUint16();
				encryptor = new SubSpaceCrypt(reader.ReadInt32());
				encrypting = true;
				std::cout << "         -- SubSpace Encryption --" << std::endl;
			}

		}
	}
	catch(...)
	{
	}

	return 0;
}

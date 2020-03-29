#include "common.h"
#include "Network.h"
#include "Application.h"
#include "TimeLib.h"
#include "LogLib.h"

Network::Network(Application & app)
	: iApplication(app),
	  iQuitFlag(false)
{
	iConnections.Register(iSelector, Selector::KRead);
	Start();
}

Network::~Network()
{
	iQuitFlag = true;
	iSelector.Alert();
	Join();
}

void Network::Connect(const InetAddress & address, IProtocol & handler)
{
	iMutex.Lock();

	if(iHandlers.find(address) != iHandlers.end())
		throw invalid_argument("[Network::Connect]: already connected to specified host.");

	iHandlers.insert(std::make_pair(address, &handler));

	try
	{
		SSConnection & connection = iConnections.Connect(address.AsString(), address);
		handler.OnConnect(connection);
	}
	catch(const std::runtime_error &)
	{
		iMutex.Unlock();
		Error();
		return;
	}

	iSelector.Alert();
	iMutex.Unlock();
}

void Network::Disconnect(IProtocol & handler)
{
	iMutex.Lock();

	for(ProtocolHandlers::iterator i = iHandlers.begin(); i != iHandlers.end(); ++i)
		if(i->second == &handler)
		{
			SSConnection * connection = iConnections.GetConnection(i->first);
			if(connection != 0)
				connection->Disconnect();

			break;
		}

	iSelector.Alert();
	iMutex.Unlock();
}

void Network::Run()
{
	for(;;)
	{
		try
		{
			iSelector.Select(400);

			if(iSelector.IsAlerted() && iQuitFlag)
				break;

			iMutex.Lock();
			try
			{
				iConnections.Poll();
			}
			catch(...)
			{
				iMutex.Unlock();
				throw;
			}
			iMutex.Unlock();

			g_idle_add(IdleCallback, this);
		}
		catch(const std::runtime_error & error)
		{
			Logger::Instance().Log(KLogError, "{NET} %s", error.what());
			g_idle_add(ErrorCallback, this);
		}
	}
}

void Network::Error()
{
	iMutex.Lock();
	for(ProtocolHandlers::iterator i = iHandlers.begin(); i != iHandlers.end(); )
	{
		ProtocolHandlers::iterator tmp = i++;

		try
		{
			SSConnection * connection = iConnections.GetConnection(tmp->first);
			if(connection != 0)
				connection->Disconnect();
		}
		catch(const std::runtime_error &)
		{
		}

		//
		// We must erase from the map before calling OnDisconnect so that the
		// callback can establish a new connection without violating any invariants.
		//
		IProtocol * handler = tmp->second;
		iHandlers.erase(tmp);
		handler->OnDisconnect();
	}

	iMutex.Unlock();

	iApplication.NetworkDied();
}

void Network::Poll()
{
	iMutex.Lock();

	for(ProtocolHandlers::iterator i = iHandlers.begin(); i != iHandlers.end(); )
	{
		SSConnection * connection = iConnections.GetConnection(i->first);
		if(connection == 0)
		{
			ProtocolHandlers::iterator tmp = i++;
			iHandlers.erase(tmp);

			//
			// Not sure if this is safe - 'tmp' is no longer in the map.
			//
			tmp->second->OnDisconnect();
			continue;
		}

		while(connection->HasMorePackets())
			i->second->OnReceive(*connection, connection->GetNextPacket());

		i->second->Poll(*connection);

		++i;
	}

	iMutex.Unlock();
}

#pragma once

#include "base.h"
#include "IProtocol.h"
#include "SSNetLib.h"
#include "ThreadLib.h"

#include <map>
#include <gtk/gtk.h>

class Application;

class Network : public Thread
{
	public:
		explicit Network(Application & app);
		~Network();

		void Connect(const InetAddress & address, IProtocol & handler);
		void Disconnect(IProtocol & handler);

	private:
		void Run();

		typedef std::map <InetAddress, IProtocol *> ProtocolHandlers;

		static gboolean IdleCallback(gpointer self) { ((Network *)self)->Poll(); return false; }
		static gboolean ErrorCallback(gpointer self) { ((Network *)self)->Error(); return false; }
		void Error();
		void Poll();

		Application & iApplication;
		Selector iSelector;
		SSConnectionPool iConnections;
		ProtocolHandlers iHandlers;
		bool iQuitFlag;
		Mutex iMutex;
};

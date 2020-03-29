#pragma once

#include "base.h"

class SSConnection;

class IProtocol
{
	public:
		virtual void Poll(SSConnection & connection) = 0;

		virtual void OnConnect(SSConnection & connection) = 0;
		virtual void OnReceive(SSConnection & connection, const bstring & packet) = 0;
		virtual void OnDisconnect() = 0;

		virtual ~IProtocol() {}
};

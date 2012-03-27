/*
  This file is part of Ingen.
  Copyright 2007-2012 David Robillard <http://drobilla.net/>

  Ingen is free software: you can redistribute it and/or modify it under the
  terms of the GNU Affero General Public License as published by the Free
  Software Foundation, either version 3 of the License, or any later version.

  Ingen is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
  A PARTICULAR PURPOSE.  See the GNU Affero General Public License for details.

  You should have received a copy of the GNU Affero General Public License
  along with Ingen.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef INGEN_ENGINE_HTTPENGINERECEIVER_HPP
#define INGEN_ENGINE_HTTPENGINERECEIVER_HPP

#include <stdint.h>

#include <string>

#include "raul/Thread.hpp"

typedef struct _SoupServer SoupServer;
typedef struct _SoupMessage SoupMessage;
typedef struct SoupClientContext SoupClientContext;

namespace Ingen {

class ServerInterface;

namespace Server {

class Engine;

class HTTPEngineReceiver
{
public:
	HTTPEngineReceiver(Engine&                    engine,
	                   SharedPtr<ServerInterface> interface,
	                   uint16_t                   port);

	~HTTPEngineReceiver();

private:
	struct ReceiveThread : public Raul::Thread {
		explicit ReceiveThread(HTTPEngineReceiver& receiver)
			: _receiver(receiver)
		{}
		virtual void _run();
	private:
		HTTPEngineReceiver& _receiver;
	};

	friend class ReceiveThread;

	static void message_callback(SoupServer*         server,
	                             SoupMessage*        msg,
	                             const char*         path,
	                             GHashTable         *query,
	                             SoupClientContext*  client,
	                             void*               data);

	Engine&                    _engine;
	SharedPtr<ServerInterface> _interface;
	ReceiveThread*             _receive_thread;
	SoupServer*                _server;
};

} // namespace Server
} // namespace Ingen

#endif // INGEN_ENGINE_HTTPENGINERECEIVER_HPP

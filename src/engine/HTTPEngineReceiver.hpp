/* This file is part of Ingen.
 * Copyright (C) 2008 Dave Robillard <http://drobilla.net>
 * 
 * Ingen is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 * 
 * Ingen is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef HTTPENGINERECEIVER_H
#define HTTPENGINERECEIVER_H

#include "config.h"
#include <string>
#include <stdint.h>
#include <libsoup/soup.h>
#include <raul/SharedPtr.hpp>
#include "QueuedEngineInterface.hpp"

namespace Ingen {

class HTTPEngineReceiver : public QueuedEngineInterface
{
public:
	HTTPEngineReceiver(Engine& engine, uint16_t port);
	~HTTPEngineReceiver();

	void activate();
	void deactivate();

private:
	struct ReceiveThread : public Raul::Thread {
		ReceiveThread(HTTPEngineReceiver& receiver) : _receiver(receiver) {}
		virtual void _run();
	private:
		HTTPEngineReceiver& _receiver;
	};

	friend class ReceiveThread;

	static void message_callback(SoupServer* server, SoupMessage* msg, const char* path,
			GHashTable *query, SoupClientContext* client, void* data);

	ReceiveThread* _receive_thread;
	SoupServer* _server;
};


} // namespace Ingen

extern "C" {
	/// Module interface
	extern Ingen::HTTPEngineReceiver* new_http_receiver(
			Ingen::Engine& engine, uint16_t port);
}

#endif // HTTPENGINERECEIVER_H

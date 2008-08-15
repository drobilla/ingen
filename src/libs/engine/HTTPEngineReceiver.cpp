/* This file is part of Ingen.
 * Copyright (C) 2007 Dave Robillard <http://drobilla.net>
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

#include <iostream>
#include <cstdlib>
#include <string>
#include "types.hpp"
#include <raul/SharedPtr.hpp>
#include <raul/AtomLiblo.hpp>
#include "interface/ClientInterface.hpp"
#include "module/Module.hpp"
#include "serialisation/serialisation.hpp"
#include "serialisation/Serialiser.hpp"
#include "engine/ThreadManager.hpp"
#include "HTTPEngineReceiver.hpp"
#include "QueuedEventSource.hpp"
#include "ClientBroadcaster.hpp"
#include "ObjectStore.hpp"

using namespace std;

namespace Ingen {


HTTPEngineReceiver::HTTPEngineReceiver(Engine& engine, uint16_t port)
	: QueuedEngineInterface(engine, 2, 2)
	, _server(soup_server_new(SOUP_SERVER_PORT, port, NULL))
{
	_receive_thread = new ReceiveThread(*this);

	soup_server_add_handler(_server, NULL, message_callback, this, NULL);

	cout << "Started HTTP server on port " << soup_server_get_port(_server) << endl;
	Thread::set_name("HTTP receiver");
	
	if (!engine.world()->serialisation_module)
		engine.world()->serialisation_module = Ingen::Shared::load_module("ingen_serialisation");

	if (engine.world()->serialisation_module)
		if (!engine.world()->serialiser)
			engine.world()->serialiser = SharedPtr<Serialiser>(
					Ingen::Serialisation::new_serialiser(engine.world()));

	if (!engine.world()->serialiser)
		cerr << "WARNING: Failed to load ingen_serialisation module, HTTP disabled." << endl;
}


HTTPEngineReceiver::~HTTPEngineReceiver()
{
	deactivate();

	if (_server != NULL)  {
		soup_server_quit(_server);
		_server = NULL;
	}
}


void
HTTPEngineReceiver::activate()
{
	QueuedEventSource::activate();
	_receive_thread->set_name("HTTP Receiver");
	_receive_thread->start();
}


void
HTTPEngineReceiver::deactivate()
{
	cout << "[HTTPEngineReceiver] Stopped HTTP listening thread" << endl;
	_receive_thread->stop();
	QueuedEventSource::deactivate();
}


void
HTTPEngineReceiver::message_callback(SoupServer* server, SoupMessage* msg, const char* path,
			GHashTable *query, SoupClientContext* client, void* data)
{
	HTTPEngineReceiver* me = (HTTPEngineReceiver*)data;

	if (msg->method != SOUP_METHOD_GET) {
		soup_message_set_status (msg, SOUP_STATUS_NOT_IMPLEMENTED);
		return;
	}
	
	SharedPtr<Serialiser> serialiser = me->_engine.world()->serialiser;
	if (!serialiser) {
		soup_message_set_status (msg, SOUP_STATUS_INTERNAL_SERVER_ERROR);
		return;
	}

	// FIXME: not thread safe!

	SharedPtr<Store> store = me->_engine.world()->store;
	assert(store);
	if (!Path::is_valid(path)) {
		soup_message_set_status (msg, SOUP_STATUS_BAD_REQUEST);
		return;
	}
	
	Store::Objects::const_iterator start = store->find(path);
	if (start == store->objects().end()) {
		soup_message_set_status (msg, SOUP_STATUS_NOT_FOUND);
		return;
	}

#if 0
	ObjectStore::Objects::iterator end = store->objects().find_descendants_end(start);

	string response;
	for (ObjectStore::Objects::iterator i = start; i != end; ++i)
		response.append(i->first).append("\n");
#endif

	const string response = serialiser->to_string(start->second,
			"http://example.org/whatever", GraphObject::Variables());
	soup_message_set_status (msg, SOUP_STATUS_OK);
	soup_message_set_response (msg, "text/plain", SOUP_MEMORY_COPY,
			response.c_str(), response.length());
}


/** Override the semaphore driven _run method of QueuedEngineInterface
 * to wait on HTTP requests and process them immediately in this thread.
 */
void
HTTPEngineReceiver::ReceiveThread::_run()
{
	soup_server_run(_receiver._server);
}


} // namespace Ingen

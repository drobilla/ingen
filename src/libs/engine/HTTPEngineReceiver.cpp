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
#include "EngineStore.hpp"

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

	if (msg->method != SOUP_METHOD_GET && msg->method != SOUP_METHOD_PUT) {
		soup_message_set_status (msg, SOUP_STATUS_NOT_IMPLEMENTED);
		return;
	}
	
	SharedPtr<Store> store = me->_engine.world()->store;
	assert(store);
	if (!Path::is_valid(path)) {
		soup_message_set_status (msg, SOUP_STATUS_BAD_REQUEST);
		return;
	}

	if (msg->method == SOUP_METHOD_GET) {
		Glib::RWLock::ReaderLock lock(store->lock());
		
		// Find object
		Store::const_iterator start = store->find(path);
		if (start == store->end()) {
			soup_message_set_status (msg, SOUP_STATUS_NOT_FOUND);
			return;
		}
		
		// Get serialiser
		SharedPtr<Serialiser> serialiser = me->_engine.world()->serialiser;
		if (!serialiser) {
			soup_message_set_status (msg, SOUP_STATUS_INTERNAL_SERVER_ERROR);
			return;
		}

		// Serialise object
		string base_uri = string("ingen:").append(start->second->path());
		const string response = serialiser->to_string(start->second, base_uri,
				GraphObject::Variables());
		soup_message_set_status (msg, SOUP_STATUS_OK);
		soup_message_set_response (msg, "text/plain", SOUP_MEMORY_COPY,
				response.c_str(), response.length());
	
	} else if (msg->method == SOUP_METHOD_PUT) {
		Glib::RWLock::WriterLock lock(store->lock());
		
		// Be sure object doesn't exist
		Store::const_iterator start = store->find(path);
		if (start != store->end()) {
			soup_message_set_status (msg, SOUP_STATUS_CONFLICT);
			return;
		}
		
		soup_message_set_status (msg, SOUP_STATUS_NOT_IMPLEMENTED);
	}
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

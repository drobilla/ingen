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
#include <cstdio>
#include <string>
#include <boost/format.hpp>
#include "raul/SharedPtr.hpp"
#include "types.hpp"
#include "interface/ClientInterface.hpp"
#include "module/Module.hpp"
#include "serialisation/serialisation.hpp"
#include "serialisation/Serialiser.hpp"
#include "serialisation/Parser.hpp"
#include "engine/ThreadManager.hpp"
#include "HTTPEngineReceiver.hpp"
#include "QueuedEventSource.hpp"
#include "ClientBroadcaster.hpp"
#include "EngineStore.hpp"

using namespace std;
using namespace Ingen::Shared;

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

	if (engine.world()->serialisation_module) {
		if (!engine.world()->serialiser)
			engine.world()->serialiser = SharedPtr<Serialiser>(
					Ingen::Serialisation::new_serialiser(engine.world(), engine.engine_store()));
		
		if (!engine.world()->parser)
			engine.world()->parser = SharedPtr<Parser>(
					Ingen::Serialisation::new_parser());
	} else {
		cerr << "WARNING: Failed to load ingen_serialisation module, HTTP disabled." << endl;
	}
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

	SharedPtr<Store> store = me->_engine.world()->store;
	if (!store) {
		soup_message_set_status (msg, SOUP_STATUS_INTERNAL_SERVER_ERROR);
		return;
	}

	if (!Path::is_valid(path)) {
		soup_message_set_status (msg, SOUP_STATUS_BAD_REQUEST);
		const string& err = (boost::format("Bad path: %1%") % path).str();
		soup_message_set_response (msg, "text/plain", SOUP_MEMORY_COPY,
				err.c_str(), err.length());
		return;
	}

	if (msg->method == SOUP_METHOD_GET) {
		Glib::RWLock::ReaderLock lock(store->lock());
		
		// Find object
		Store::const_iterator start = store->find(path);
		if (start == store->end()) {
			soup_message_set_status (msg, SOUP_STATUS_NOT_FOUND);
			const string& err = (boost::format("No such object: %1%") % path).str();
			soup_message_set_response (msg, "text/plain", SOUP_MEMORY_COPY,
					err.c_str(), err.length());
			return;
		}
		
		// Get serialiser
		SharedPtr<Serialiser> serialiser = me->_engine.world()->serialiser;
		if (!serialiser) {
			soup_message_set_status (msg, SOUP_STATUS_INTERNAL_SERVER_ERROR);
			soup_message_set_response (msg, "text/plain", SOUP_MEMORY_STATIC,
					"No serialiser available\n", 24);
			return;
		}

#if 0
		SoupMessageHeaders* in_head = msg->request_headers;
		const char* str = soup_message_headers_get(in_head, "Accept");
		cout << "Accept: " << str << endl;
#endif

		// Serialise object
		const string response = serialiser->to_string(start->second,
				"http://example.org", GraphObject::Variables());

#if 0
		FILE* xhtml_file = fopen("/home/dave/ingen_ui.xhtml", "r");
		string response;
		while (!feof(xhtml_file)) {
			int c = fgetc(xhtml_file);
			if (c != EOF)
				response += (char)c;
		}
		fclose(xhtml_file);
#endif
		
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
		
		// Get parser
		SharedPtr<Parser> parser = me->_engine.world()->parser;
		if (!parser) {
			soup_message_set_status (msg, SOUP_STATUS_INTERNAL_SERVER_ERROR);
			return;
		}
		
		//cout << "POST: " << msg->request_body->data << endl;

		// Load object
		soup_message_set_status (msg, SOUP_STATUS_NOT_IMPLEMENTED);
	} else if (msg->method == SOUP_METHOD_POST) {
		//cout << "PUT: " << msg->request_body->data << endl;
		soup_message_set_status (msg, SOUP_STATUS_NOT_IMPLEMENTED);
	} else {
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


extern "C" {

Ingen::HTTPEngineReceiver*
new_http_receiver(Ingen::Engine& engine, uint16_t port)
{
	return new Ingen::HTTPEngineReceiver(engine, port);
}

} // extern "C"


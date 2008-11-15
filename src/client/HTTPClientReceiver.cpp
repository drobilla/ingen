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

#include <list>
#include <cassert>
#include <cstring>
#include <iostream>
#include <sstream>
#include "module/Module.hpp"
#include "HTTPClientReceiver.hpp"

using namespace std;
using namespace Raul;

namespace Ingen {
namespace Client {


HTTPClientReceiver::HTTPClientReceiver(
		Shared::World*                     world,
		const std::string&                 url,
		SharedPtr<Shared::ClientInterface> target)
	: _target(target)
	, _world(world)
	, _url(url)
	, _session(NULL)
{
	start(false);
}


HTTPClientReceiver::~HTTPClientReceiver()
{
	stop();
}


void
HTTPClientReceiver::Listener::_run()
{
#if 0
	cout << "LISTENER RUN" << endl;
	/*const string uri = "http://localhost:16180";
	SoupMessage* msg = soup_message_new("GET", (uri + "/stream").c_str());
	soup_message_headers_set_encoding(msg->response_headers, SOUP_ENCODING_CHUNKED);
	soup_session_send_message(_session, msg);*/
	
	size_t offset = 0;
	soup_message_body_set_accumulate(_msg->response_body, false);
	while (true) {
		SoupBuffer* chunk = soup_message_body_get_chunk(_msg->response_body, offset);
		if (chunk == NULL) {
			//cout << "WAITING FOR DATA" << endl;
		} else if (chunk->length == 0) {
			cout << "CHUNKED TRANSFER COMPLETED" << endl;
			break;
		} else {
			cout << "RECEIVED CHUNK: " << (char*)chunk->data << endl;
			offset += chunk->length;
		}
	}

	cout << "LISTENER FINISHED" << endl;
#endif
}


void
HTTPClientReceiver::message_callback(SoupSession* session, SoupMessage* msg, void* ptr)
{
	HTTPClientReceiver* me = (HTTPClientReceiver*)ptr;
	const string path = soup_message_get_uri(msg)->path;
	cout << "MESSAGE: " << path << endl;
	if (path == "/") {
		me->_target->response_ok(0);
		me->_target->enable();
	} else if (path == "/plugins") {
		if (msg->response_body->data == NULL) {
			cout << "ERROR: Empty response" << endl;
		} else {
			me->_target->response_ok(0);
			me->_target->enable();
			me->_parser->parse_string(me->_world, me->_target.get(),
					Glib::ustring(msg->response_body->data),
					Glib::ustring("."), Glib::ustring(""));
		}
	} else if (path == "/patch") {
		if (msg->response_body->data == NULL) {
			cout << "ERROR: Empty response" << endl;
		} else {
			me->_target->response_ok(0);
			me->_target->enable();
			me->_parser->parse_string(me->_world, me->_target.get(),
					Glib::ustring(msg->response_body->data),
					Glib::ustring("/patch/"), Glib::ustring(""));
		}
	} else if (path == "/stream") {
		cout << "STREAM" << endl;
		//me->_listener = boost::shared_ptr<Listener>(new Listener(me->_session, msg));
		//me->_listener->start();
	} else {
		cerr << "UNKNOWN MESSAGE: " << path << endl;
	}
}


void
HTTPClientReceiver::start(bool dump)
{
	Glib::Mutex::Lock lock(_world->rdf_world->mutex());
	if (!_parser) {
		if (!_world->serialisation_module)
			_world->serialisation_module = Ingen::Shared::load_module("ingen_serialisation");

		if (_world->serialisation_module) {
			Parser* (*new_parser)() = NULL;
			if (_world->serialisation_module->get_symbol("new_parser", (void*&)new_parser))
				_parser = SharedPtr<Parser>(new_parser());
		}
	}
	
	_session = soup_session_sync_new();
	SoupMessage* msg;
	
	msg = soup_message_new("GET", _url.c_str());
	soup_session_queue_message(_session, msg, message_callback, this);
	
	msg = soup_message_new("GET", (_url + "/plugins").c_str());
	soup_session_queue_message(_session, msg, message_callback, this);
	
	msg = soup_message_new("GET", (_url + "/patch").c_str());
	soup_session_queue_message(_session, msg, message_callback, this);
	
	msg = soup_message_new("GET", (_url + "/stream").c_str());
	soup_session_queue_message(_session, msg, message_callback, this);
}


void
HTTPClientReceiver::stop()
{
	if (_session != NULL) {
		//unregister_client();
		soup_session_abort(_session);
		_session = NULL;
	}
}


} // namespace Client
} // namespace Ingen

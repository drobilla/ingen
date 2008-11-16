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
#include <sys/socket.h>
#include <errno.h>
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


HTTPClientReceiver::Listener::~Listener()
{
	close(_sock);
}

HTTPClientReceiver::Listener::Listener(HTTPClientReceiver* receiver, const std::string uri)
	: _uri(uri)
	, _receiver(receiver)
{
	string port_str = uri.substr(uri.find_last_of(":")+1);
	int port = atoi(port_str.c_str());

	cout << "HTTP listen URI: " << uri << " port: " << port << endl;

	struct sockaddr_in servaddr;
	
	// Create listen address
	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family      = AF_INET;
	servaddr.sin_port        = htons(port);
	
	// Create listen socket
	if ((_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		cerr << "Error creating listening socket: %s" << strerror(errno) << endl;
		_sock = -1;
		return;
	}

	// Set remote address (FIXME: always localhost)
	if (inet_aton("127.0.0.1", &servaddr.sin_addr) <= 0) {
		cerr << "Invalid remote IP address" << endl;
		_sock = -1;
		return;
	}

	// Connect to server
	if (connect(_sock, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0) {
		cerr << "Error calling connect: " << strerror(errno) << endl;
		_sock = -1;
		return;
	}
}

void
HTTPClientReceiver::update(const std::string& str)
{
	cout << "UPDATE: " << str << endl;
	cout << _parser->parse_string(_world, _target.get(), str, "/", "/");
}

void
HTTPClientReceiver::Listener::_run()
{
	char   in    = '\0';
	char   last  = '\0';
	char   llast = '\0';
	string recv  = "";

	while (true) {
		while (read(_sock, &in, 1) > 0 ) {
			recv += in;
			if (in == '\n' && last == '\n' && llast == '\n') {
				if (recv != "") {
					_receiver->update(recv);
					recv = "";
					last = '\0';
					llast = '\0';
				}
				break;
			}
			llast = last;
			last = in;
		}
	}

	cout << "HTTP listener finished" << endl;
}


void
HTTPClientReceiver::message_callback(SoupSession* session, SoupMessage* msg, void* ptr)
{
	HTTPClientReceiver* me = (HTTPClientReceiver*)ptr;
	const string path = soup_message_get_uri(msg)->path;
	if (path == "/") {
		me->_target->response_ok(0);
		me->_target->enable();

	
	} else if (path == "/plugins") {
		if (msg->response_body->data == NULL) {
			cout << "ERROR: Empty response" << endl;
		} else {
			Glib::Mutex::Lock lock(me->_mutex);
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
			Glib::Mutex::Lock lock(me->_mutex);
			me->_target->response_ok(0);
			me->_target->enable();
			me->_parser->parse_string(me->_world, me->_target.get(),
					Glib::ustring(msg->response_body->data),
					Glib::ustring("/patch/"), Glib::ustring(""));
		}

	} else if (path == "/stream") {
		if (msg->response_body->data == NULL) {
			cout << "ERROR: Empty response" << endl;
		} else {
			Glib::Mutex::Lock lock(me->_mutex);
			string uri = string(soup_uri_to_string(soup_message_get_uri(msg), false));
			uri = uri.substr(0, uri.find_last_of(":"));
			uri += string(":") + msg->response_body->data;
			cout << "Stream URI: " << uri << endl;
			me->_listener = boost::shared_ptr<Listener>(new Listener(me, uri));
			me->_listener->start();
		}

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


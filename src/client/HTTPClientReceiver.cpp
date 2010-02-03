/* This file is part of Ingen.
 * Copyright (C) 2008-2009 Dave Robillard <http://drobilla.net>
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
#include <sstream>
#include <sys/socket.h>
#include <errno.h>
#include "raul/log.hpp"
#include "raul/Atom.hpp"
#include "module/Module.hpp"
#include "module/World.hpp"
#include "HTTPClientReceiver.hpp"

#define LOG(s) s << "[HTTPClientReceiver] "

using namespace std;
using namespace Raul;

namespace Ingen {

using namespace Serialisation;

namespace Client {

static SoupSession*        client_session  = NULL;
static HTTPClientReceiver* client_receiver = NULL;

HTTPClientReceiver::HTTPClientReceiver(
		Shared::World*                     world,
		const std::string&                 url,
		SharedPtr<Shared::ClientInterface> target)
	: _target(target)
	, _world(world)
	, _url(url)
{
	start(false);
	client_receiver = this;
}


HTTPClientReceiver::~HTTPClientReceiver()
{
	stop();
	if (client_receiver == this)
		client_receiver = NULL;
}


HTTPClientReceiver::Listener::Listener(HTTPClientReceiver* receiver, const std::string uri)
	: _uri(uri)
	, _receiver(receiver)
{
	const string port_str = uri.substr(uri.find_last_of(":")+1);
	int port = atoi(port_str.c_str());

	LOG(info) << "Client HTTP listen: " << uri << " (port " << port << ")" << endl;

	struct sockaddr_in servaddr;

	// Create listen address
	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family      = AF_INET;
	servaddr.sin_port        = htons(port);

	// Create listen socket
	if ((_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		LOG(error) << "Error creating listening socket: %s" << strerror(errno) << endl;
		_sock = -1;
		return;
	}

	// Set remote address (FIXME: always localhost)
	if (inet_aton("127.0.0.1", &servaddr.sin_addr) <= 0) {
		LOG(error) << "Invalid remote IP address" << endl;
		_sock = -1;
		return;
	}

	// Connect to server
	if (connect(_sock, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0) {
		LOG(error) << "Error calling connect: " << strerror(errno) << endl;
		_sock = -1;
		return;
	}
}


HTTPClientReceiver::Listener::~Listener()
{
	close(_sock);
}

void
HTTPClientReceiver::send(SoupMessage* msg)
{
	if (!client_session) {
		LOG(debug) << "Starting session" << endl;
		client_session = soup_session_sync_new();
	}

	assert(SOUP_IS_MESSAGE(msg));
	soup_session_queue_message(client_session, msg, message_callback, client_receiver);
}


void
HTTPClientReceiver::close_session()
{
	if (client_session) {
		SoupSession* s = client_session;
		client_session = NULL;
		soup_session_abort(s);
	}
}


void
HTTPClientReceiver::update(const std::string& str)
{
	LOG(info) << _world->parser->parse_update(_world, _target.get(), str, _url);
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

	LOG(info) << "HTTP listener finished" << endl;
}


void
HTTPClientReceiver::message_callback(SoupSession* session, SoupMessage* msg, void* ptr)
{
	if (ptr == NULL)
		return;

	HTTPClientReceiver* me = (HTTPClientReceiver*)ptr;
	const string path = soup_message_get_uri(msg)->path;

	/*LOG(debug) << path << endl
			<< msg->response_body->data << endl;*/

	if (msg->response_body->data == NULL) {
		LOG(error) << "Empty client message" << endl;
		return;
	}

	if (path == Path::root.str()) {
		me->_target->response_ok(0);

	} else if (path == "/plugins") {
		if (msg->response_body->data == NULL) {
			LOG(error) << "Empty response" << endl;
		} else {
			Glib::Mutex::Lock lock(me->_mutex);
			me->_target->response_ok(0);
			me->_world->parser->parse_string(me->_world, me->_target.get(),
					Glib::ustring(msg->response_body->data), me->_url);
		}

	} else if (path == "/patch") {
		if (msg->response_body->data == NULL) {
			LOG(error) << "Empty response" << endl;
		} else {
			Glib::Mutex::Lock lock(me->_mutex);
			me->_target->response_ok(0);
			me->_world->parser->parse_string(me->_world, me->_target.get(),
					Glib::ustring(msg->response_body->data),
					Glib::ustring("/patch/"));
		}

	} else if (path == "/stream") {
		if (msg->response_body->data == NULL) {
			LOG(error) << "Empty response" << endl;
		} else {
			Glib::Mutex::Lock lock(me->_mutex);
			string uri = string(soup_uri_to_string(soup_message_get_uri(msg), false));
			uri = uri.substr(0, uri.find_last_of(":"));
			uri += string(":") + msg->response_body->data;
			LOG(info) << "Stream URI: " << uri << endl;
			me->_listener = boost::shared_ptr<Listener>(new Listener(me, uri));
			me->_listener->start();
		}

	} else {
		LOG(error) << "Unknown message: " << path << endl;
		me->update(msg->response_body->data);
	}
}


void
HTTPClientReceiver::start(bool dump)
{
	Glib::Mutex::Lock lock(_world->rdf_world->mutex());

	if (!_world->parser)
		_world->load("ingen_serialisation");

	SoupMessage* msg = soup_message_new("GET", (_url + "/stream").c_str());
	assert(SOUP_IS_MESSAGE(msg));
	soup_session_queue_message(client_session, msg, message_callback, this);
}


void
HTTPClientReceiver::stop()
{
	//unregister_client();
	close_session();
}


} // namespace Client
} // namespace Ingen


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

#include "HTTPSender.hpp"
#include <cassert>
#include <iostream>
#include <unistd.h>
#include <stdarg.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <errno.h>

#include <cstring>

using namespace std;

namespace Ingen {
namespace Shared {


HTTPSender::HTTPSender()
	: _listen_port(-1)
	, _listen_sock(-1)
	, _client_sock(-1)
	, _send_state(Immediate)
{
	Thread::set_name("HTTP Sender");

	struct sockaddr_in addr;

	// Create listen address
	memset(&addr, 0, sizeof(addr));
	addr.sin_family      = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_ANY);

	// Create listen socket
	if ((_listen_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0 ) {
		fprintf(stderr, "Error creating listening socket (%s)\n", strerror(errno));
		exit(EXIT_FAILURE);
	}

	// Bind our socket addresss to the listening socket
	if (bind(_listen_sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
		fprintf(stderr, "Error calling bind (%s)\n", strerror(errno));
		_listen_sock = -1;
	}
	
	// Find port number
	socklen_t length = sizeof(addr);
	if (getsockname(_listen_sock, (struct sockaddr*)&addr, &length) == -1) {
		fprintf(stderr, "Error calling getsockname (%s)\n", strerror(errno));
		_listen_sock = -1;
		return;
	}
   
	if (listen(_listen_sock, 1) < 0 ) {
		cerr << "Error calling listen: %s" << strerror(errno) << endl;
		_listen_sock = -1;
		return;
	}

	_listen_port = ntohs(addr.sin_port);
	cout << "Opening event stream on TCP port " << _listen_port << endl;
	start();
}


HTTPSender::~HTTPSender()
{
	stop();
	if (_listen_sock != -1)
		close(_listen_sock);
	if (_client_sock != -1)
		close(_client_sock);
}

void
HTTPSender::_run()
{
	if (_listen_sock == -1) {
		cerr << "Unable to open socket, exiting sender thread" << endl;
		return;
	}
	
	// Accept connection
	if ((_client_sock = accept(_listen_sock, NULL, NULL) ) < 0) {
		cerr << "Error calling accept: " << strerror(errno) << endl;
		return;
	}

	// Hold connection open and write when signalled
	while (true) {
		_mutex.lock();
		_signal.wait(_mutex);

		write(_client_sock, _transfer.c_str(), _transfer.length());
		write(_client_sock, "\n\n\n", 3);

		_signal.broadcast();
		_mutex.unlock();
	}

	close(_listen_sock);
	_listen_sock = -1;
}


void
HTTPSender::bundle_begin()
{
	_mutex.lock();
	_send_state = SendingBundle;
	_transfer = "";
}


void
HTTPSender::bundle_end()
{
	assert(_send_state == SendingBundle);
	_signal.broadcast();
	_signal.wait(_mutex);
	_send_state = Immediate;
	_mutex.unlock();
}

	
void
HTTPSender::send_chunk(const std::string& buf)
{
	if (_send_state == Immediate) {
		_mutex.lock();
		_transfer = buf;
		_signal.broadcast();
		_signal.wait(_mutex);
		_mutex.unlock();
	} else {
		_transfer.append(buf);
	}
}


} // namespace Shared
} // namespace Ingen

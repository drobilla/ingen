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

using namespace std;

namespace Ingen {
namespace Shared {


HTTPSender::HTTPSender(SoupServer* server, SoupMessage* msg)
	: _server(server)
	, _msg(msg)
{
	soup_message_headers_set_encoding(msg->response_headers, SOUP_ENCODING_CHUNKED);
	cout << "Hello?" << endl;
	send_chunk("hello");
}


HTTPSender::~HTTPSender()
{
	cout << "HTTP SENDER EXIT" << endl;
	soup_message_body_complete(_msg->response_body);
	soup_server_unpause_message(_server, _msg);
}


void
HTTPSender::bundle_begin()
{
	_send_state = SendingBundle;
}


void
HTTPSender::bundle_end()
{
	assert(_send_state == SendingBundle);
	soup_message_body_append(_msg->response_body,
			SOUP_MEMORY_TEMPORARY, _transfer.c_str(), _transfer.length());
	soup_server_unpause_message(_server, _msg);
	_transfer = "";
	_send_state = Immediate;
}

	
void
HTTPSender::send_chunk(const std::string& buf)
{
	if (_send_state == Immediate) {
		soup_message_body_append(_msg->response_body,
				SOUP_MEMORY_TEMPORARY, buf.c_str(), buf.length());
		soup_server_unpause_message(_server, _msg);
	} else {
		_transfer.append(buf);
	}
}


} // namespace Shared
} // namespace Ingen
